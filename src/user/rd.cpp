#include "rd.h"
#include "rtl.h"
#include "parser.h"

size_t __stdcall rd(const kiv_hal::TRegisters &regs) {

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	kiv_os_rtl::Delete_File(input);

	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	
	kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
	char* message = "";
	if (error != kiv_os::NOS_Error::Success)
	{
		getErrorMessage(error, message);
	}
	else {
		message = "Removed.\n";
	}
	kiv_os_rtl::Write_File(std_out, message, strlen(message), counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
