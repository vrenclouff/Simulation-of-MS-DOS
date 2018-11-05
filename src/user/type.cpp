#include "type.h"
#include "rtl.h"

size_t __stdcall type(const kiv_hal::TRegisters &regs) {

	char* input = reinterpret_cast<char*>(regs.rdi.r);
	const char* linebreak = "\n";
	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	kiv_os_rtl::Write_File(std_out, input, strlen(input), counter);
	kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
