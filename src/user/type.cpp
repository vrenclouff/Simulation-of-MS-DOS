#include "type.h"
#include "rtl.h"
#include "error.h"

#include <filesystem>
#include <iostream>
#include <sstream>

size_t __stdcall type(const kiv_hal::TRegisters &regs) {

	size_t written;
	size_t read;
	std::string linebreak = "\n";

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r)); 

	if (input.empty()) {
		const size_t buffer_size = 256;
		char buffer[buffer_size];
		size_t read;
		std::vector<std::string> elements;
		do
		{
			if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, read)) {
				kiv_os_rtl::Write_File(std_out, linebreak.c_str(), linebreak.length(), written);
				buffer[read] = 0;
				elements.push_back(buffer);
			}
		} while (read != 0);

		for (std::vector<std::string>::const_iterator i = elements.begin(); i != elements.end(); ++i)
			std::cout << *i << linebreak;
	}
	else {
		kiv_os::THandle filehandle;

		if (!kiv_os_rtl::Open_File(input.data(), input.size(), filehandle, true, kiv_os::NFile_Attributes::Read_Only)) {
			const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
			const std::string error_msg = Error_Message(error);
			kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), written);
		
			const auto error_code = static_cast<uint16_t>(error);
			kiv_os_rtl::Exit(error_code);
			return error_code;
		}

		char content[2048];
		do {
			kiv_os_rtl::Read_File(filehandle, content, sizeof(content), read);
			kiv_os_rtl::Write_File(std_out, content, read, written);
		} while (read);

		kiv_os_rtl::Write_File(std_out, linebreak.c_str(), linebreak.length(), written);

		kiv_os_rtl::Close_Handle(filehandle);
	}
	
	kiv_os_rtl::Exit(0);
	return 0;
}
