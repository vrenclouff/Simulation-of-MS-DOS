#include "ps.h"
#include "rtl.h"
#include "error.h"

size_t __stdcall ps(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = std::string_view("A:/procfs");

	size_t read, written;
	kiv_os::THandle filehandle;
	if (!kiv_os_rtl::Open_File(input.data(), input.size(), filehandle, true, kiv_os::NFile_Attributes::Read_Only)) {
		const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
		const auto error_msg = Error_Message(error);
		const auto error_code = static_cast<uint16_t>(error);

		kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.length(), written);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	char buffer[1024];
	kiv_os_rtl::Read_File(filehandle, buffer, sizeof buffer, read);
	buffer[read] = 0;
	kiv_os_rtl::Write_File(std_out, buffer, read, written);

	kiv_os_rtl::Exit(0);
	return 0;
}
