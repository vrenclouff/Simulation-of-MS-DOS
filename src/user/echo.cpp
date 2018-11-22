#include "echo.h"
#include "rtl.h"

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
	const std::string notImplementedYet = "Not implemented yet.\n";

	if ((!first.empty() && !first.compare("on")) && (rest.empty() || !rest.compare("")))
	{
		kiv_os_rtl::Write_File(std_out, notImplementedYet.c_str(), notImplementedYet.length(), written);
		kiv_os_rtl::Exit(1);
		return 1;
	}
	
	else if ((!first.empty() && !first.compare("off")) && (rest.empty() || !rest.compare("")))
	{
		kiv_os_rtl::Write_File(std_out, notImplementedYet.c_str(), notImplementedYet.length(), written);
		kiv_os_rtl::Exit(1);
		return 1;
	}

	else {
		kiv_os_rtl::Write_File(std_out, input, strlen(input), written);
	}

	const std::string linebreak = "\n";
	kiv_os_rtl::Write_File(std_out, linebreak.c_str(), linebreak.length(), written);

	kiv_os_rtl::Exit(0);
	return 0;
}
