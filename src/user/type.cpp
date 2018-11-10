#include "type.h"
#include "rtl.h"

#include <filesystem>

size_t __stdcall type(const kiv_hal::TRegisters &regs) {
	   
	//std::string input = std::string(reinterpret_cast<char*>(regs.rdi.r));
	std::string input = "lorem2.txt";

	kiv_os::THandle file_handle;
	kiv_os_rtl::Open_File(input.c_str(), input.size(), file_handle, false);
	

	const kiv_os::THandle console_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	size_t counter;
	kiv_os_rtl::Write_File(console_out, input.c_str(), input.size(), counter);
	const char* linebreak = "\n";
	kiv_os_rtl::Write_File(console_out, linebreak, strlen(linebreak), counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
