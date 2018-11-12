#include "ps.h"
#include "rtl.h"

size_t __stdcall ps(const kiv_hal::TRegisters &regs) {

	const auto input = "A:/procfs";
	const auto input_size = strlen(input);

	kiv_os::THandle filehandle;
	kiv_os_rtl::Open_File(input, input_size, filehandle, true, std::iostream::ios_base::in);

	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	if (!regs.flags.carry || filehandle) {
		char content[256];
		kiv_os_rtl::Read_File(filehandle, content, sizeof(content), counter);
		kiv_os_rtl::Write_File(std_out, content, counter, counter);
	}
	else {
		// TODO error
	}

	kiv_os_rtl::Write_File(std_out, "\n", 1, counter);

	kiv_os_rtl::Exit(0);
	return 0;
}
