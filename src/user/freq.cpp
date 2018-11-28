#include "freq.h"
#include "rtl.h"
#include "error.h"

#include <map>

size_t __stdcall freq(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

	std::map<unsigned char, int> frequencies;
	char buffer[1024];
	size_t read, written;
	do {
		if (kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read)) {
			for (auto i = 0; i < read; i++) {
				frequencies[buffer[i]]++;
			}
		}
		else {
			const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
			const auto error_msg = Error_Message(error);
			const auto error_code = static_cast<uint16_t>(error);

			kiv_os_rtl::Write_File(std_out, error_msg.c_str(), error_msg.length(), written);
			kiv_os_rtl::Exit(error_code);
			return error_code;
		}
	} while (read);

	char out[255];
	for (const auto& item : frequencies) {
		sprintf_s(out, sizeof out, "\n0x%hhx : %d", item.first, item.second);
		kiv_os_rtl::Write_File(std_out, out, strlen(out), written);
	}
	
	kiv_os_rtl::Write_File(std_out, "\n", 1, written);

	kiv_os_rtl::Exit(0);
	return 0;
}
