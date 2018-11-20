#include "ps.h"
#include "rtl.h"
#include "parser.h"

size_t __stdcall ps(const kiv_hal::TRegisters &regs) {

	size_t read, wrote;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = std::string_view("A:/procfs");

	kiv_os::THandle filehandle;
	if (!kiv_os_rtl::Open_File(input.data(), input.size(), filehandle, true, kiv_os::NFile_Attributes::Read_Only)) {
		const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
		char* error_msg = "";
		getErrorMessage(error, error_msg);
		kiv_os_rtl::Write_File(std_out, error_msg, strlen(error_msg), wrote);
		const auto error_code = static_cast<uint16_t>(error);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	char content[1024];
	kiv_os_rtl::Read_File(filehandle, content, sizeof(content), read);
	kiv_os_rtl::Write_File(std_out, content, read, wrote);

	kiv_os_rtl::Exit(0);
	return 0;
}
