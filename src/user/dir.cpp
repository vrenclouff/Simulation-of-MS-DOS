#include "dir.h"
#include "rtl.h"
#include "error.h"

#define MAX_ENTRY_ITEMS	 16

size_t __stdcall dir(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = reinterpret_cast<char*>(regs.rdi.r);

	size_t read, wrote;
	char promot[100];
	if (input[0] == 0) {
		kiv_os_rtl::Get_Working_Dir(promot, sizeof promot, read);
	}
	else {
		const auto path_len = strlen(input);
		std::copy(input, input + path_len, promot);
		read = path_len;
	}

	kiv_os::THandle dirhandle;
	if (kiv_os_rtl::Open_File(promot, read, dirhandle, true, kiv_os::NFile_Attributes::Read_Only)) {
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
					const auto type_size = strlen(type);
					std::copy(type, type + type_size, formatted_buffer + seek);
					seek += type_size;

					formatted_buffer[seek++] = '\t';

					const auto access = dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only) ? "R" : "R/W";
					const auto access_size = strlen(access);
					std::copy(access, access + access_size, formatted_buffer + seek);
					seek += access_size;

					formatted_buffer[seek++] = '\n';
				}
			}
			else {
				const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
				const auto error_msg = Error_Message(error);
				const auto error_code = static_cast<uint16_t>(error);

				kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.size(), wrote);
				kiv_os_rtl::Exit(error_code);
				return error_code;
			}
		} while (read);
		kiv_os_rtl::Close_Handle(dirhandle);
		kiv_os_rtl::Write_File(std_out, formatted_buffer, seek, wrote);
	}
	else {
		const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
		const auto error_msg = Error_Message(error);
		const auto error_code = static_cast<uint16_t>(error);

		kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.size(), wrote);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	kiv_os_rtl::Exit(0);

	return 0;
}
