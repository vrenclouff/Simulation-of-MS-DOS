#include "wc.h"
#include "rtl.h"

#include <string>
#include <sstream>
#include <iterator>
#include <vector>

size_t __stdcall wc(const kiv_hal::TRegisters &regs) {
	std::string input = std::string(reinterpret_cast<char*>(regs.rdi.r));

	std::istringstream is(input);
	std::string first;
	std::string second;
	char space = ' ';

	std::getline(is, first, space);
	std::getline(is, second, space);

	size_t written;
	const std::string linebreak = "\n";
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	if ((first.compare("\\v") || second.compare("\\c")) && (first.compare("\\c") || second.compare("\\v")))
	{
		const std::string incorrect_Use = "Incorrect use of find command. Try: find \\v \\c \"<some text or file>\"\n";
		kiv_os_rtl::Write_File(std_out, incorrect_Use.c_str(), incorrect_Use.length(), written);
		kiv_os_rtl::Exit(1);
		return 1;
	}

	std::string arg;
	std::getline(is, arg);

	std::istringstream iswords(arg);
	std::string skip;
	char quote = '"';
	std::getline(std::getline(iswords, skip, quote), arg, quote);

	std::string output;
	if (!arg.empty())
	{
		std::stringstream stream(arg);
		std::istream_iterator<std::string> begin(stream);
		std::istream_iterator<std::string> end;

		std::vector<std::string> elements(begin, end);

		std::string wordcount = std::to_string(elements.size());
		output = wordcount;
	}
	else {
		output = "0";
	}

	kiv_os_rtl::Write_File(std_out, output.c_str(), output.length(), written);
	kiv_os_rtl::Write_File(std_out, linebreak.c_str(), linebreak.length(), written);

	kiv_os_rtl::Exit(0);
	return 0;
}
