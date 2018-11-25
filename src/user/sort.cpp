#include "sort.h"
#include "rtl.h"

#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>

size_t __stdcall sort(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

	// char* input = reinterpret_cast<char*>(regs.rdi.r);

	char buffer[1024];
	size_t read, written;
	std::vector<std::string> elements;
	do {
		if (!kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read)) {
			// TODO error
		}

		buffer[read] = 0;
		elements.push_back(buffer);

	} while (read == sizeof buffer);

	std::sort(elements.begin(), elements.end());

	// kiv_os_rtl::Write_File(std_out, "\n", 1, written);

	for (std::string item : elements) {
		kiv_os_rtl::Write_File(std_out, item.c_str(), item.length(), written);
	}

	kiv_os_rtl::Exit(0);
	return 0;
}
