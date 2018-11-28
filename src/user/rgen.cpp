#include "rgen.h"
#include "rtl.h"
#include "error.h"

#include <random>
#include <ctime>
#include <cfloat>
#include <limits>

void wait_For_Ctrlz(const kiv_hal::TRegisters &regs) {

	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	bool* generating = reinterpret_cast<bool*>(regs.rdi.r);

	char buffer[1];
	size_t read = 1;
	while (read) {
		kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read);
	}

	*generating = false;
	kiv_os_rtl::Exit(0);
	return;
}

size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {

	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	size_t written;
	if (!input.empty()) {
		const kiv_os::NOS_Error error = kiv_os::NOS_Error::Invalid_Argument;
		const auto error_msg = Error_Message(error);
		const auto error_code = static_cast<uint16_t>(error);

		kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), written);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	bool generating = true;

	kiv_os::THandle handler = kiv_os_rtl::Create_Thread(&wait_For_Ctrlz, &generating, std_in, std_out);

	std::random_device rd;
	std::mt19937 generator(rd());

	char fltout[sizeof(float) * 8];
	while (generating) {
		float rndflt = std::generate_canonical<float, std::numeric_limits<float>::digits>(generator);
		sprintf_s(fltout, "%f\n", rndflt);
		kiv_os_rtl::Write_File(std_out, fltout, strlen(fltout), written);
	}

	kiv_os_rtl::Wait_For(&handler);
	kiv_os_rtl::Read_Exit_Code(handler);

	kiv_os_rtl::Exit(0);
	return 0;
}
