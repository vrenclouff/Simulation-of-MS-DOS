#include "md.h"
#include "rtl.h"

size_t __stdcall md(const kiv_hal::TRegisters &regs) {

	const char* notImplementedYet = "Not implemented yet.\n";
	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	kiv_os_rtl::Write_File(std_out, notImplementedYet, strlen(notImplementedYet), counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
