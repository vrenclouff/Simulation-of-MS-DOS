#include "rgen.h"
#include "rtl.h"

#include <random>
#include <ctime>
#include <cfloat>
#include <limits>
#include <sstream>

void wait_For_Ctrlz(const kiv_hal::TRegisters &regs) {

	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	bool* generating = reinterpret_cast<bool*>(regs.rdi.r);

	char read;
	size_t count;
	while (true)
	{
		kiv_os_rtl::Read_File(std_in, &read, sizeof(read), count);

		if (read == '\0')
		{
			*generating = false;
			kiv_os_rtl::Exit(0);
			return;
		}
	}
}

size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {

	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	size_t counter;

	std::istringstream is(reinterpret_cast<char*>(regs.rdi.r));
	std::string arg;

	std::getline(is, arg, ' ');

	if (!arg.empty()) {
		std::string too_Much_Arguments = "Rgen function has no arguments.\n";
		kiv_os_rtl::Write_File(std_out, too_Much_Arguments.c_str(), too_Much_Arguments.length(), counter);
		kiv_os_rtl::Exit(1);
		return 1;
	}

	bool generating = true;

	kiv_os::THandle handler = kiv_os_rtl::Create_Thread(&wait_For_Ctrlz, &generating, std_in, std_out);

	std::mt19937 generator((unsigned int) time(0));

	while (generating) {

		float rndflt = std::generate_canonical<float, std::numeric_limits<float>::digits>(generator);

		char fltout[64];
		sprintf_s(fltout, "%f\n", rndflt);
		kiv_os_rtl::Write_File(std_out, fltout, strlen(fltout), counter);
	}

	kiv_os_rtl::Wait_For(&handler);
	kiv_os_rtl::Read_Exit_Code(handler);

	kiv_os_rtl::Exit(0);
	return 0;
}
