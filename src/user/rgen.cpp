#include "rgen.h"
#include "rtl.h"
#include "error.h"

#include <random>
#include <ctime>
#include <cfloat>
#include <limits>

bool rgen_exit_signaled = false;

void rgen_Sigterm_Handler(const kiv_hal::TRegisters &regs) {
	rgen_exit_signaled = true;
}

void ctrlZ_Sigterm_Handler(const kiv_hal::TRegisters &regs) {
	//kiv_os_rtl::Exit(0);
}

void wait_For_Ctrlz(const kiv_hal::TRegisters &regs) {
	kiv_os_rtl::Register_Signal_Handler(kiv_os::NSignal_Id::Terminate, reinterpret_cast<kiv_os::TThread_Proc>(ctrlZ_Sigterm_Handler));
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	bool* generating = reinterpret_cast<bool*>(regs.rdi.r);

	char buffer[1];
	size_t read = 1;
	while (read) {
		kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read);
		if (buffer[0] == 4)
		{
			break;
		}
	}

	if (!rgen_exit_signaled)
	{
		*generating = false;
		kiv_os_rtl::Exit(0);
	}
}

size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {

	kiv_os_rtl::Register_Signal_Handler(kiv_os::NSignal_Id::Terminate, reinterpret_cast<kiv_os::TThread_Proc>(rgen_Sigterm_Handler));

	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = reinterpret_cast<const char*>(regs.rdi.r);

	size_t written;
	if (*input) {
		const kiv_os::NOS_Error error = kiv_os::NOS_Error::Invalid_Argument;
		const auto error_msg = Error_Message(error);
		const auto error_code = static_cast<uint16_t>(error);

		kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), written);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	bool generating = true;
	const auto handler = kiv_os_rtl::Create_Thread(&wait_For_Ctrlz, &generating, std_in, std_out);

	std::random_device rd;
	std::mt19937 generator(rd());

	char fltout[sizeof(float) * 8];
	while (generating && !rgen_exit_signaled) {
		float rndflt = std::generate_canonical<float, std::numeric_limits<float>::digits>(generator);
		sprintf_s(fltout, "%f\n", rndflt);
		kiv_os_rtl::Write_File(std_out, fltout, strlen(fltout), written);
	}

	if (!rgen_exit_signaled)
	{
		kiv_os_rtl::Wait_For(handler);
		kiv_os_rtl::Read_Exit_Code(handler);
	}

	kiv_os_rtl::Exit(0);
	return 0;
}
