#include "md.h"
#include "rtl.h"

size_t __stdcall md(const kiv_hal::TRegisters &regs) {

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	kiv_os::THandle filehandle;
	kiv_os_rtl::Open_File(input, sizeof(input), filehandle, true, std::iostream::ios_base::out);

	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	char* message = "";
	if (regs.flags.carry == 1 || filehandle == NULL)
	{
		message = "Something went wrong.\n";
	}
	else {
		message = "Created.\n";
	}
	kiv_os_rtl::Write_File(std_out, message, strlen(message), counter);

	kiv_os_rtl::Exit(0);	
	return 0;
}
