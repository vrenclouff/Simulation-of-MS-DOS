#include "type.h"
#include "rtl.h"

#include <filesystem>
#include <iostream>
#include <sstream>

size_t __stdcall type(const kiv_hal::TRegisters &regs) {

	size_t written;
	size_t read;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const char* linebreak = "\n";
	   
	auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	std::istringstream is(input);
	std::string filename;
	std::getline(is, filename, ' ');

	if (!filename.empty()) {
		kiv_os::THandle filehandle;
		kiv_os_rtl::Open_File(input.c_str(), input.size(), filehandle, true, std::iostream::ios_base::in);

		const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

		char content[1124];
		do {
			kiv_os_rtl::Read_File(filehandle, content, sizeof(content), read);
			kiv_os_rtl::Write_File(std_out, content, read, written);
		} while (read);
	}
	else {
		const size_t buffer_size = 256;
		char buffer[buffer_size];
		size_t read;
		std::vector<std::string> elements;
		do
		{
			if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, read)) {
				kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), written);
				buffer[read] = 0;
				elements.push_back(buffer);
			}
		} while (read != 0);

		for (std::vector<std::string>::const_iterator i = elements.begin(); i != elements.end(); ++i)
			std::cout << *i << linebreak;
	}
	

	kiv_os_rtl::Exit(0);
	return 0;
}
