#include "wc.h"
#include "rtl.h"

#include <string>
#include <sstream>
#include <iterator>
#include <vector>

size_t __stdcall wc(const kiv_hal::TRegisters &regs) {
	// TODO correct input format with ""
	char* input = reinterpret_cast<char*>(regs.rdi.r);

	char* first = strtok_s(input, " ", &input);
	char* second = strtok_s(input, " ", &input);

	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	size_t counter;

	if ((first != NULL && second != NULL) && ((!strcmp(first, "\\v") && !strcmp(second, "\\c")) || (!strcmp(first, "\\c") && !strcmp(second, "\\v"))))
	{
		std::string words(input);

		std::stringstream stream(words);
		std::istream_iterator<std::string> begin(stream);
		std::istream_iterator<std::string> end;

		std::vector<std::string> elements(begin, end);

		std::string s = std::to_string(elements.size());
		char const *pchar = s.c_str();

		const char* linebreak = "\n";
		const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
		kiv_os_rtl::Write_File(std_out, pchar, strlen(pchar), counter);
		kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);
	}
	else
	{
		const char* notImplementedYet = "Incorrect use of find command. Try: find \\v \\c \"<some text or file>\"\n";
		kiv_os_rtl::Write_File(std_out, notImplementedYet, strlen(notImplementedYet), counter);
	}

	return 0;
}
