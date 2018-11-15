#include "md.h"
#include "rtl.h"
#include "parser.h";

size_t __stdcall md(const kiv_hal::TRegisters &regs) {

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	kiv_os::THandle filehandle;
	kiv_os_rtl::Open_File(input, sizeof(input), filehandle, false, std::iostream::ios_base::out);

	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	char* message = "";

	kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
	if (error != kiv_os::NOS_Error::Success || filehandle == NULL)
	{
		getErrorMessage(error, message);
	}
	else {
		message = "Created.\n";
	}
	kiv_os_rtl::Write_File(std_out, message, strlen(message), counter);

	kiv_os_rtl::Exit(0);	
	return 0;
}
