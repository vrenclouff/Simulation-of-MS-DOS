#include "type.h"
#include "rtl.h"

#include <filesystem>
#include <iostream>
#include <sstream>

size_t __stdcall type(const kiv_hal::TRegisters &regs) {

	size_t wrote, read;
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	if (input.empty()) {
		const kiv_os::NOS_Error error = kiv_os::NOS_Error::Invalid_Argument;
		char* error_msg = "";
		kiv_os_rtl::Write_File(std_out, error_msg, strlen(error_msg), wrote);
		const auto error_code = static_cast<uint16_t>(error);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	kiv_os::THandle filehandle;

	if (!kiv_os_rtl::Open_File(input.data(), input.size(), filehandle, true, kiv_os::NFile_Attributes::Read_Only)) {
		// TODO predelat na Last_error
		const auto error_msg = std::string_view("Cannot find the file.");
		kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.size(), wrote);
		
		const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
		const auto error_code = static_cast<uint16_t>(error);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	char content[2048];
	do {
		kiv_os_rtl::Read_File(filehandle, content, sizeof(content), read);
		kiv_os_rtl::Write_File(std_out, content, read, wrote);
	} while (read);

	kiv_os_rtl::Write_File(std_out, "\n", 1, wrote);

	kiv_os_rtl::Close_Handle(filehandle);
	
	kiv_os_rtl::Exit(0);
	return 0;
}
