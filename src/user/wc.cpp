#include "wc.h"
#include "rtl.h"

#include <string>
#include <sstream>
#include <iterator>
#include <vector>

size_t __stdcall wc(const kiv_hal::TRegisters &regs) {
	char* input = reinterpret_cast<char*>(regs.rdi.r);

	char* first = strtok_s(input, " ", &input);
	char* second = strtok_s(input, " ", &input);

	size_t counter;
	const char* linebreak = "\n";
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	if ((first == NULL || second == NULL) || ((strcmp(first, "\\v") || strcmp(second, "\\c")) && (strcmp(first, "\\c") || strcmp(second, "\\v"))))
	{
		const char* notImplementedYet = "Incorrect use of find command. Try: find \\v \\c \"<some text or file>\"\n";
		kiv_os_rtl::Write_File(std_out, notImplementedYet, strlen(notImplementedYet), counter);
		return 1;
	}

	char* arg = strtok_s(input, "\"", &input);

	char const *output;
	if (arg != NULL)
	{
		std::string words(arg);

		std::stringstream stream(words);
		std::istream_iterator<std::string> begin(stream);
		std::istream_iterator<std::string> end;

		std::vector<std::string> elements(begin, end);

		std::string wordcount = std::to_string(elements.size());
		output = wordcount.c_str();
	}
	else {
		output = "0";
	}

	kiv_os_rtl::Write_File(std_out, output, strlen(output), counter);
	kiv_os_rtl::Write_File(std_out, linebreak, strlen(linebreak), counter);

	return 0;
}
