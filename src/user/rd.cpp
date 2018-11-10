#include "rd.h"
#include "rtl.h"

size_t __stdcall rd(const kiv_hal::TRegisters &regs) {

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	kiv_os_rtl::Delete_File(input);

	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	char* message = "";
	if (regs.flags.carry == 1)
	{
		message = "Could not find specified directory.\n";
	}
	else {
		message = "Removed.\n";
	}
	kiv_os_rtl::Write_File(std_out, message, strlen(message), counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
