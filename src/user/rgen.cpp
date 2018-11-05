#include "rgen.h"
#include "rtl.h"

#include <random>
#include <ctime>
#include <cfloat>
#include <limits>

size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {
	//TODO ctrl+z

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

	std::mt19937 generator(time(0));

	float rndflt = std::generate_canonical<float, std::numeric_limits<float>::digits>(generator);

	char fltout[sizeof(rndflt)];
	snprintf(fltout, sizeof(rndflt), "%f\n", rndflt);
	kiv_os_rtl::Write_File(std_out, fltout, strlen(fltout), counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
