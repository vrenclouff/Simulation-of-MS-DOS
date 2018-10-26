#include "rgen.h"
#include "rtl.h"

#include <cstdlib>
#include <ctime>

size_t __stdcall rgen(const kiv_hal::TRegisters &regs) {
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	size_t counter;

	char fltout[64];
	float rndfloat = float(rand() % 1000) / 100;
	snprintf(fltout, sizeof fltout, "%f", rndfloat);
	kiv_os_rtl::Write_File(std_out, fltout, strlen(fltout), counter);

	const char* linebreak = "\n";
	kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);

	return 0;
}
