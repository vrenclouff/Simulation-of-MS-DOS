#include "echo.h"
#include "rtl.h"

size_t __stdcall echo(const kiv_hal::TRegisters &regs) {

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	char* rest;
	char* first = strtok_s(input, " ", &rest);

	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	size_t counter;
	const char* notImplementedYet = "Not implemented yet.";

	if (first != NULL && !strcmp(first, "on"))
	{
		kiv_os_rtl::Write_File(std_out, notImplementedYet, strlen(notImplementedYet), counter);
	}
	
	else if (first != NULL && !strcmp(first, "off"))
	{
		kiv_os_rtl::Write_File(std_out, notImplementedYet, strlen(notImplementedYet), counter);
	}

	else {
		kiv_os_rtl::Write_File(std_out, input, strlen(input), counter);
	}

	const char* linebreak = "\n";
	kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);

	return 0;
}
