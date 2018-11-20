#include "type.h"
#include "rtl.h"

#include <filesystem>
#include <iostream>
#include <sstream>

size_t __stdcall type(const kiv_hal::TRegisters &regs) {

	size_t wrote, read;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	   
	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	std::string filename;
	std::getline(std::istringstream(input), filename, ' ');

	if (!filename.empty()) {
		kiv_os::THandle filehandle;

		if (!kiv_os_rtl::Open_File(input.c_str(), input.size(), filehandle, true, std::iostream::ios_base::in)) {
			const auto error_msg = std::string_view("Cannot find the file.");
			kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.size(), wrote);
			return 1;
		}

		char content[2048];
		do {
			kiv_os_rtl::Read_File(filehandle, content, sizeof(content), read);
			kiv_os_rtl::Write_File(std_out, content, read, wrote);
		} while (read);

		kiv_os_rtl::Write_File(std_out, "\n", 1, wrote);
		kiv_os_rtl::Close_Handle(filehandle);
	}
	else {
		const auto error_msg = std::string_view("Input is empty.\nUse: type <file_path>\n");
		kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.size(), wrote);
		return 1;
	}
	
	kiv_os_rtl::Exit(0);
	return 0;
}
