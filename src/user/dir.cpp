#include "dir.h"
#include "rtl.h"
#include "error.h"

#include <vector>
#include <sstream>

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
		const std::string error_msg = Error_Message(error);
		kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), wrote);

		const auto error_code = static_cast<uint16_t>(error);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	std::stringstream ss;

	ss << "RO\tHIDDEN\tSYSTEM\tVOLUME\tIS DIR\tARCHIVE\tFILENAME\n";

	char buffer[MAX_ENTRY_ITEMS * sizeof(kiv_os::TDir_Entry)];
	do {
		kiv_os_rtl::Read_File(dirhandle, buffer, sizeof(buffer), read);

		if (!read) break;

		for (size_t beg = 0; beg < read; beg += sizeof(kiv_os::TDir_Entry)) {

			const auto dir = reinterpret_cast<kiv_os::TDir_Entry*>(buffer + beg);
			const auto file_attributes = static_cast<kiv_os::NFile_Attributes>(dir->file_attributes);

			const auto filename = std::string(dir->file_name, sizeof kiv_os::TDir_Entry::file_name);
			const auto read_only = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only));
			const auto hidden = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Hidden));
			const auto system_file = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File));
			const auto volume_id = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID));
			const auto is_dir = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory));
			const auto archive = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Archive));

			ss
				<< read_only << "\t"
				<< hidden << "\t"
				<< system_file << "\t"
				<< volume_id << "\t"
				<< is_dir << "\t"
				<< archive << "\t"
				<< filename << "\n"
			;
		}

		const auto text = ss.str();
		kiv_os_rtl::Write_File(std_out, text.c_str(), text.size(), wrote);

	} while (1);

	kiv_os_rtl::Close_Handle(dirhandle);
	kiv_os_rtl::Exit(0);

	return 0;
}
