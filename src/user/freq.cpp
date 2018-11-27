#include "freq.h"
#include "rtl.h"

#include <map>
#include <sstream>

size_t __stdcall freq(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

	//const auto input = reinterpret_cast<char*>(regs.rdi.r);

	std::stringstream ss;
	char buffer[1024];
	size_t read, written;
	do
	{
		if (kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read)) {
			kiv_os_rtl::Write_File(std_out, "\n", 1, written);
			buffer[read] = 0;
			ss << buffer;
		}
	} while (read);

	const auto bytes = ss.str();
	std::map<unsigned char, int> frequencies;
	for (int i = 0; i < bytes.size(); i++) {
		frequencies[bytes[i]]++;
	}

	char out[255];
	for (const auto& item : frequencies) {
		sprintf_s(out, sizeof out, "0x%hhx : %d\n", item.first, item.second);
		kiv_os_rtl::Write_File(std_out, out, strlen(out), written);
	}
	
	kiv_os_rtl::Exit(0);
	return 0;
}
