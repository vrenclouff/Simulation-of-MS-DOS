#include "echo.h"
#include "rtl.h"
#include "shell.h"

#include <sstream>

size_t __stdcall echo(const kiv_hal::TRegisters &regs) {

	const char* input = reinterpret_cast<char*>(regs.rdi.r);
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	
	std::istringstream is(input);
	std::string first;
	std::string rest;
	char space = ' ';

	std::getline(is, first, space);
	std::getline(is, rest);

	size_t written;

	if (first.empty() || !first.compare("")) {
		std::string echo_info = "Echo is ";
		echo_info.append(shell_echo ? "on." : "off." );
		kiv_os_rtl::Write_File(std_out, echo_info.c_str(), echo_info.length(), written);
	}
	else if (!first.compare("on") && (rest.empty() || !rest.compare("")))
	{
		shell_echo = true;
	}
	
	else if (!first.compare("off") && (rest.empty() || !rest.compare("")))
	{
		shell_echo = false;
	}

	else {
		kiv_os_rtl::Write_File(std_out, input, strlen(input), written);
	}

	const std::string linebreak = "\n";
	kiv_os_rtl::Write_File(std_out, linebreak.c_str(), linebreak.length(), written);

	kiv_os_rtl::Exit(0);
	return 0;
}
