#include "type.h"
#include "rtl.h"

#include <filesystem>

size_t __stdcall type(const kiv_hal::TRegisters &regs) {
	   
	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	kiv_os::THandle filehandle;
	kiv_os_rtl::Open_File(input.c_str(), input.size(), filehandle, true, std::iostream::ios_base::in);

	size_t read_counter, write_counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	if (kiv_os_rtl::Last_Error != kiv_os::NOS_Error::Success || filehandle == NULL) {
		kiv_os_rtl::Write_File(std_out, input.c_str(), input.size(), write_counter);
	}
	else {
		char content[1124];
		do {
			kiv_os_rtl::Read_File(filehandle, content, sizeof(content), read_counter);
			kiv_os_rtl::Write_File(std_out, content, read_counter, write_counter);
		} while (read_counter);
	}

	kiv_os_rtl::Write_File(std_out, "\n", 1, write_counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
