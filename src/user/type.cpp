#include "type.h"
#include "rtl.h"

#include <filesystem>

size_t __stdcall type(const kiv_hal::TRegisters &regs) {
	   
	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	kiv_os::THandle filehandle;
	kiv_os_rtl::Open_File(input.c_str(), input.size(), filehandle, true, std::iostream::ios_base::in);

	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const char* linebreak = "\n";

	char* message = "";
	if (regs.flags.carry == 1 || filehandle == NULL)
	{
		kiv_os_rtl::Write_File(std_out, input.c_str(), input.size(), counter);
	}
	else {
		char content[1024];
		size_t read;
		kiv_os_rtl::Read_File(filehandle, content, sizeof(content), read);
		content[read] = '\0';
		kiv_os_rtl::Write_File(std_out, content, strlen(content), counter);
	}
	kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
