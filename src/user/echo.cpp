#include "echo.h"
#include "rtl.h"
#include <stdlib.h>

size_t __stdcall echo(const kiv_hal::TRegisters &regs) {

	const char* input = reinterpret_cast<char*>(regs.rdi.r);

	char* first = "";
	char* rest;
	size_t inputsize = strlen(input) + 1;
	char* tokenize = new char[inputsize];

	strcpy_s(tokenize, strlen(tokenize), input);

	first = strtok_s(tokenize, " ", &rest);

	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	size_t counter;
	const char* notImplementedYet = "Not implemented yet.\n";

	if (first != NULL && !strcmp(first, "on") && (rest == NULL || !strcmp(rest, "")))
	{
		kiv_os_rtl::Write_File(std_out, notImplementedYet, strlen(notImplementedYet), counter);
		kiv_os_rtl::Exit(1);
		return 1;
	}
	
	else if (first != NULL && !strcmp(first, "off") && (rest == NULL || !strcmp(rest, "")))
	{
		kiv_os_rtl::Write_File(std_out, notImplementedYet, strlen(notImplementedYet), counter);
		kiv_os_rtl::Exit(1);
		return 1;
	}

	else {
		kiv_os_rtl::Write_File(std_out, input, strlen(input), counter);
	}

	const char* linebreak = "\n";
	kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
