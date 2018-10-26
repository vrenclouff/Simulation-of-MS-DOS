#include "freq.h"
#include "rtl.h"

#include <map>

size_t __stdcall freq(const kiv_hal::TRegisters &regs) {

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	std::map<unsigned char, int> frequencies;
	for (int i = 0; i < strlen(input); ++i) {
		++frequencies[input[i]];
	}

	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	char out[255];
	for (std::map<unsigned char, int>::iterator iter = frequencies.begin(); iter != frequencies.end(); ++iter) {
		sprintf_s(out, sizeof(out), "0x%hhx : %d\n", iter->first, iter->second);
		kiv_os_rtl::Write_File(std_out, out, strlen(out), counter);
	}
	
	return 0;
}
