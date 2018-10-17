#include "md.h"
#include "rtl.h"

size_t __stdcall md(const kiv_hal::TRegisters &regs) {

	/*const char* linebreak = "\n";
	const char* notImplementedYet = "Not implemented yet.";
	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	kiv_os_rtl::Write_File(std_out, notImplementedYet, strlen(notImplementedYet), counter);
	kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);
*/

	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	std::string file_name("lorem2.txt");

	kiv_os::THandle file_handle;
	kiv_os_rtl::Open_File(file_name.c_str(), file_name.size(), file_handle);

	char buffer[5];
	size_t counter;
	kiv_os_rtl::Read_File(file_handle, buffer, sizeof(buffer), counter);

	auto str = std::string(buffer, counter);

	const auto close_handle = kiv_os_rtl::Close_Handle(file_handle);

	kiv_os_rtl::Exit(0);	
	return 0;
}
