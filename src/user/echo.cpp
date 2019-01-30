#include "echo.h"
#include "rtl.h"
#include "shell.h"

#include <sstream>

// @author: Lukas Cerny & Lenka Simeckova

size_t __stdcall echo(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	
	const auto input = reinterpret_cast<const char*>(regs.rdi.r);

	size_t written;
	if (!(*input)) {
		std::string echo_info = "Echo is ";
		echo_info.append(shell_echo ? "on." : "off." );
		kiv_os_rtl::Write_File(std_out, echo_info.c_str(), echo_info.length(), written);
	}

	auto str_input = std::string(input);
	if (str_input.compare("on") == 0) {
		shell_echo = true;
		kiv_os_rtl::Exit(0);
		return 0;
	}

	if (str_input.compare("off") == 0) {
		shell_echo = false;
		kiv_os_rtl::Exit(0);
		return 0;
	}

	str_input.append("\n");
	kiv_os_rtl::Write_File(std_out, str_input.data(), str_input.size(), written);

	kiv_os_rtl::Exit(0);
	return 0;
}
