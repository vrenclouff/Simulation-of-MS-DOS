#include "sort.h"
#include "rtl.h"

#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>

size_t __stdcall sort(const kiv_hal::TRegisters &regs) {
	size_t counter;
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const char* linebreak = "\n";

	char* input = reinterpret_cast<char*>(regs.rdi.r);

	std::string words(input);

	std::stringstream stream(words);
	std::istream_iterator<std::string> begin(stream);
	std::istream_iterator<std::string> end;

	std::vector<std::string> elements(begin, end);

	std::sort(elements.begin(), elements.end());

	for (std::string item : elements) {
		const char* out = item.c_str();
		kiv_os_rtl::Write_File(std_out, out, strlen(out), counter);
		kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);
	}

	kiv_os_rtl::Exit(0);
	return 0;
}
