#include "md.h"
#include "rtl.h"
#include "parser.h"

size_t __stdcall md(const kiv_hal::TRegisters &regs) {
	
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const auto input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	kiv_os::THandle filehandle;

	if (!kiv_os_rtl::Open_File(input.data(), input.size(), filehandle, false, kiv_os::NFile_Attributes::Directory)) {
		// TODO error
		const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
		const auto error_code = static_cast<uint16_t>(error);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}

	kiv_os_rtl::Close_Handle(filehandle);

	kiv_os_rtl::Exit(0);	
	return 0;
}
