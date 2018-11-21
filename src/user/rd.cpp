#include "rd.h"
#include "rtl.h"
#include "parser.h"

size_t __stdcall rd(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = reinterpret_cast<char*>(regs.rdi.r);

	size_t wrote;
	if (!kiv_os_rtl::Delete_File(input)) {
		// TODO error msg
		const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
		const auto error_msg = std::string_view("<error_msg>\n");
		const auto error_code = static_cast<uint16_t>(error);
		kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.size(), wrote);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	kiv_os_rtl::Write_File(std_out, "\n", 1, wrote);

	kiv_os_rtl::Exit(0);
	return 0;
}
