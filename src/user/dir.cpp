#include "dir.h"
#include "rtl.h"
#include "error.h"

#include <vector>
#include <sstream>
#include <mutex>

#define MAX_ENTRY_ITEMS	 16

size_t __stdcall dir(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	size_t read, wrote;
	std::string path = input;
	if (input.empty()) {
		char buffer[100];
		kiv_os_rtl::Get_Working_Dir(buffer, sizeof(buffer), read);
		path = std::string(buffer, read);
	}

	kiv_os::THandle dirhandle;
	if (!kiv_os_rtl::Open_File(path.data(), path.size(), dirhandle, true, kiv_os::NFile_Attributes::Read_Only)) {
		const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
		const auto error_msg = Error_Message(error);
		const auto error_code = static_cast<uint16_t>(error);

		kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), wrote);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}
	char buffer[MAX_ENTRY_ITEMS * sizeof(kiv_os::TDir_Entry)];
	char formatted_buffer[512]; size_t seek = 0;
	do {
		if (kiv_os_rtl::Read_File(dirhandle, buffer, sizeof(buffer), read)) {

			for (size_t beg = 0; beg < read; beg += sizeof(kiv_os::TDir_Entry)) {
				const auto dir = reinterpret_cast<kiv_os::TDir_Entry*>(buffer + beg);

				std::copy(dir->file_name, dir->file_name + sizeof kiv_os::TDir_Entry::file_name, formatted_buffer + seek);
				seek += sizeof kiv_os::TDir_Entry::file_name;

				formatted_buffer[seek++] = '\t';

				const auto type = dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory) ? "<DIR>" : "<FILE>";
				std::copy(type, type + strlen(type), formatted_buffer + seek);
				seek += strlen(type);

				formatted_buffer[seek++] = '\t';

				const auto access = dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only) ? "R" : "R/W";
				std::copy(access, access + strlen(access), formatted_buffer + seek);
				seek += strlen(access);

				formatted_buffer[seek++] = '\n';
			}
		}
		else {
			const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
			const auto error_msg = Error_Message(error);
			const auto error_code = static_cast<uint16_t>(error);

			kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), wrote);
			kiv_os_rtl::Exit(error_code);
			return error_code;
		}
	} while (read);
	kiv_os_rtl::Close_Handle(dirhandle);

	kiv_os_rtl::Write_File(std_out, formatted_buffer, seek, wrote);

	kiv_os_rtl::Exit(0);

	return 0;
}
