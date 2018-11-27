#include "freq.h"
#include "rtl.h"

#include <map>

size_t __stdcall freq(const kiv_hal::TRegisters &regs) {

	size_t written;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const std::string linebreak = "\n";

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	std::string bytes = "";

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t read;
	do
	{
		if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, read)) {
			buffer[read] = 0;
			bytes.append(buffer);
		}
	} while (read);

	std::map<unsigned char, int> frequencies;
	for (int i = 0; i < bytes.size(); i++) {
		frequencies[bytes[i]]++;
	}

	char out[255];
	for (std::map<unsigned char, int>::iterator iter = frequencies.begin(); iter != frequencies.end(); ++iter) {
		sprintf_s(out, sizeof(out), "0x%hhx : %d\n", iter->first, iter->second);
		kiv_os_rtl::Write_File(std_out, out, strlen(out), written);
	}
	
	kiv_os_rtl::Exit(0);
	return 0;
}
