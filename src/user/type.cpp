#include "type.h"
#include "rtl.h"
#include "error.h"

#include <filesystem>
#include <iostream>
#include <sstream>

size_t __stdcall type(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r)); 
	
	size_t read, written;
	char buffer[2048];
	if (input.empty()) {
		std::vector<std::string> elements;
		do {
			if (kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read)) {
				buffer[read] = 0;
				elements.push_back(buffer);
			}
		} while (read);

		std::stringstream lines;
		std::copy(elements.begin(), elements.end(), std::ostream_iterator<std::string>(lines, ""));
		const auto res = lines.str();
		kiv_os_rtl::Write_File(std_out, res.c_str(), res.length(), written);

	}
	else {
		kiv_os::THandle filehandle;
		if (!kiv_os_rtl::Open_File(input.data(), input.size(), filehandle, true, kiv_os::NFile_Attributes::Read_Only)) {
			const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
			const auto error_msg = Error_Message(error);
			const auto error_code = static_cast<uint16_t>(error);

			kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), written);
			kiv_os_rtl::Exit(error_code);
			return error_code;
		}

		std::stringstream ss;
		do {
			kiv_os_rtl::Read_File(filehandle, buffer, sizeof buffer, read);
			ss << std::string(buffer, read);
		} while (read);

		const auto output = ss.str().append("\n");
		kiv_os_rtl::Write_File(std_out, output.data(), output.size(), written);
		kiv_os_rtl::Close_Handle(filehandle);
	}
	
	kiv_os_rtl::Exit(0);
	return 0;
}
