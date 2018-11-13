#include "dir.h"
#include "rtl.h"

#include <vector>

size_t __stdcall dir(const kiv_hal::TRegisters &regs) {

	const auto input = reinterpret_cast<char*>(regs.rdi.r);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	size_t counter;
	std::string path;
	if (strlen(input) == 0) {
		char buffer[100];
		kiv_os_rtl::Get_Working_Dir(buffer, sizeof(buffer), counter);
		path = std::string(buffer, counter);
	}
	else {
		path = input;
	}

	kiv_os::THandle dirhandle;
	kiv_os_rtl::Open_File(path.c_str(), path.size(), dirhandle, true, 1);

	char buffer[10 * sizeof(kiv_os::TDir_Entry)]; // space for 30 directories
	kiv_os_rtl::Read_File(dirhandle, buffer, sizeof(buffer), counter);
	buffer[counter] = '\0';

	std::string text = "FILENAME\tRO\tHIDDEN\tSYSTEM\tVOLUME\tIS DIR\tARCHIVE\n";

	for (size_t beg = 0; beg < counter; beg += sizeof(kiv_os::TDir_Entry)) {

		const auto dir = reinterpret_cast<kiv_os::TDir_Entry*>(buffer + beg);
		const auto file_attributes = static_cast<kiv_os::NFile_Attributes>(dir->file_attributes);

		auto filename = std::string(dir->file_name, sizeof kiv_os::TDir_Entry::file_name);
		auto read_only = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only));
		auto hidden = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Hidden));
		auto system_file = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::System_File));
		auto volume_id = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Volume_ID));
		auto is_dir = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Directory));
		auto archive = std::to_string(dir->file_attributes & static_cast<uint8_t>(kiv_os::NFile_Attributes::Archive));

		text.append(filename).append("\t").append(read_only).append("\t").append(hidden).append("\t").append(system_file).append("\t").append(volume_id).append("\t").append(is_dir).append("\t").append(archive).append("\n");
	}

	kiv_os_rtl::Write_File(std_out, text.c_str(), text.size(), counter);

	kiv_os_rtl::Close_Handle(dirhandle);

	kiv_os_rtl::Exit(0);
	return 0;
}
