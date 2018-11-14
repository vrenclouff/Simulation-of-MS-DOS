#include "dir.h"
#include "rtl.h"

#include <vector>
#include <sstream>

#define MAX_ENTRY_ITEMS	 30

size_t __stdcall dir(const kiv_hal::TRegisters &regs) {

	const auto input = reinterpret_cast<char*>(regs.rdi.r);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	size_t read_counter, write_counter;
	std::string path;
	if (strlen(input) == 0) {
		char buffer[100];
		kiv_os_rtl::Get_Working_Dir(buffer, sizeof(buffer), read_counter);
		path = std::string(buffer, read_counter);
	}
	else {
		path = input;
	}

	kiv_os::THandle dirhandle;
	kiv_os_rtl::Open_File(path.c_str(), path.size(), dirhandle, true, 1);

	const auto header = "RO\tHIDDEN\tSYSTEM\tVOLUME\tIS DIR\tARCHIVE\tFILENAME\n";
	kiv_os_rtl::Write_File(std_out, header, strlen(header), write_counter);

	char buffer[MAX_ENTRY_ITEMS * sizeof(kiv_os::TDir_Entry)];
	do {
		kiv_os_rtl::Read_File(dirhandle, buffer, sizeof(buffer), read_counter);

		for (size_t beg = 0; beg < read_counter; beg += sizeof(kiv_os::TDir_Entry)) {

			const auto dir = reinterpret_cast<kiv_os::TDir_Entry*>(buffer + beg);
			const auto file_attributes = static_cast<kiv_os::NFile_Attributes>(dir->file_attributes);

			const auto filename = std::string(dir->file_name, sizeof kiv_os::TDir_Entry::file_name);
			const auto read_only = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only));
			const auto hidden = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Hidden));
			const auto system_file = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File));
			const auto volume_id = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID));
			const auto is_dir = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory));
			const auto archive = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Archive));

			std::stringstream ss;
			ss
				<< read_only << "\t"
				<< hidden << "\t"
				<< system_file << "\t"
				<< volume_id << "\t"
				<< is_dir << "\t"
				<< archive << "\t"
				<< filename << "\n"
			;

			const auto text = ss.str();
			kiv_os_rtl::Write_File(std_out, text.c_str(), text.size(), write_counter);
		}
	} while (read_counter);

	kiv_os_rtl::Close_Handle(dirhandle);
	kiv_os_rtl::Exit(0);

	return 0;
}
