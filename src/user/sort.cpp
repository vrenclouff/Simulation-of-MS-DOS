#include "sort.h"
#include "rtl.h"

#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>

size_t __stdcall sort(const kiv_hal::TRegisters &regs) {
	size_t written;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const std::string linebreak = "\n";

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	std::vector<std::string> elements;

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t read;
	do
	{
		if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, read)) {
			kiv_os_rtl::Write_File(std_out, linebreak.c_str(), linebreak.length(), written);
			buffer[read] = 0;
			elements.push_back(buffer);
		}
	} while (read != 0);

	std::sort(elements.begin(), elements.end());

	for (std::string item : elements) {
		kiv_os_rtl::Write_File(std_out, item.c_str(), item.length(), written);
		kiv_os_rtl::Write_File(std_out, linebreak.c_str(), linebreak.length(), written);
	}

	kiv_os_rtl::Exit(0);
	return 0;
}
