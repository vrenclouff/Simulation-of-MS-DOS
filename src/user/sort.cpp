#include "sort.h"
#include "rtl.h"

#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>

size_t __stdcall sort(const kiv_hal::TRegisters &regs) {
	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const char* linebreak = "\n";

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	std::vector<std::string> elements;

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t read;
	do
	{
		if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, read)) {
			kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);
			buffer[read] = 0;
			elements.push_back(buffer);
		}
	} while (read != 0);

	std::sort(elements.begin(), elements.end());

	for (std::string item : elements) {
		const char* out = item.c_str();
		kiv_os_rtl::Write_File(std_out, out, strlen(out), counter);
		kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);
	}

	kiv_os_rtl::Exit(0);
	return 0;
}
