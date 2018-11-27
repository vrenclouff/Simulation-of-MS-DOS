#include "echo.h"
#include "rtl.h"
#include "shell.h"

#include <sstream>

size_t __stdcall echo(const kiv_hal::TRegisters &regs) {

	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	
	size_t written;
	std::istringstream is(input);
	std::string first;
	std::string rest;

	std::getline(is, first, ' ');
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

	/*
	if (input.compare("on") == 0) {
		shell_echo = true;
		kiv_os_rtl::Exit(0);
		return 0;
	}

	if (input.compare("off") == 0) {
		shell_echo = false;
		kiv_os_rtl::Exit(0);
		return 0;
	}
	*/

	kiv_os_rtl::Write_File(std_out, input.data(), input.size(), written);
	kiv_os_rtl::Write_File(std_out, "\n", 1, written);

	kiv_os_rtl::Exit(0);
	return 0;
}
