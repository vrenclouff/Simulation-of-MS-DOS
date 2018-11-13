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

	// TODO dodelat do hezkeho formatu

	std::string text;
	for (size_t beg = 0; beg < counter; beg += sizeof(kiv_os::TDir_Entry)) {
		const auto dir = reinterpret_cast<kiv_os::TDir_Entry*>(buffer + beg);
		text.append(std::string(dir->file_name, sizeof kiv_os::TDir_Entry::file_name)).append("\n");
	}

	kiv_os_rtl::Write_File(std_out, text.c_str(), text.size(), counter);

	kiv_os_rtl::Close_Handle(dirhandle);

	kiv_os_rtl::Exit(0);
	return 0;
}
