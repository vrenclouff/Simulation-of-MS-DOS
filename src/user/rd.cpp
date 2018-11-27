#include "rd.h"
#include "rtl.h"
#include "error.h"

size_t __stdcall rd(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	size_t written;
	if (input.empty()) {
		const auto error = kiv_os::NOS_Error::Invalid_Argument;
		const auto error_msg = Error_Message(error);
		const auto error_code = static_cast<uint16_t>(error);

		kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), written);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	if (!kiv_os_rtl::Delete_File(input.c_str())) {
		const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
		const std::string error_msg = Error_Message(error);
		kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), written);

		const auto error_code = static_cast<uint16_t>(error);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	kiv_os_rtl::Write_File(std_out, "\n", 1, written);

	kiv_os_rtl::Exit(0);
	return 0;
}
