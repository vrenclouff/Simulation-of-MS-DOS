#include "rgen.h"
#include "rtl.h"

#include <random>
#include <ctime>
#include <cfloat>
#include <limits>

void waitForCtrlz(const kiv_hal::TRegisters &regs) {

	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	bool* generating = reinterpret_cast<bool*>(regs.rdi.r);

	char* read = "";
	size_t count;
	while (true)
	{
		kiv_os_rtl::Read_File(std_in, read, sizeof(read), count);

		if (!strcmp(read, "\0"))
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

	char* input = reinterpret_cast<char*>(regs.rdi.r);
	char* arg = strtok_s(input, " ", &input);

	if (arg != NULL) {
		char* tooMuchArguments = "Rgen function has no arguments.\n";
		kiv_os_rtl::Write_File(std_out, tooMuchArguments, strlen(tooMuchArguments), counter);
		kiv_os_rtl::Exit(1);
		return 1;
	}

	bool generating = true;
	kiv_os_rtl::Create_Thread(&waitForCtrlz, &generating, std_in, std_out);

	std::mt19937 generator(time(0));

	while (generating) {

		float rndflt = std::generate_canonical<float, std::numeric_limits<float>::digits>(generator);

		char fltout[64];
		sprintf_s(fltout, "%f\n", rndflt);
		kiv_os_rtl::Write_File(std_out, fltout, strlen(fltout), counter);
	}

	kiv_os_rtl::Exit(0);
	return 0;
}
