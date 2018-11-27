#include "wc.h"
#include "rtl.h"
#include "error.h"

#include <string>
#include <sstream>
#include <iterator>
#include <vector>

size_t __stdcall wc(const kiv_hal::TRegisters &regs) {
	std::string input = std::string(reinterpret_cast<char*>(regs.rdi.r));
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

	std::istringstream is(input);
	std::string first;
	std::string second;
	std::string arg;
	char space = ' ';

	std::getline(is, first, space);
	std::getline(is, second, space);
	std::getline(is, arg);

	size_t written;
	size_t read;
	const std::string linebreak = "\n";

	if (first.compare("\\v") || second.compare("\\c\"\"") || !arg.empty())
	{
		const std::string incorrect_Use = "The syntax of the command is incorrect.\n";
		kiv_os_rtl::Write_File(std_out, incorrect_Use.c_str(), incorrect_Use.length(), written);
		kiv_os_rtl::Exit(1);
		return 1;
	}

	char buffer[1024];
	std::stringstream stream;
	do {
		if (!kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read)) {
			const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
			const auto error_msg = Error_Message(error);
			kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.length(), written);
			const auto error_code = static_cast<uint16_t>(error);
			kiv_os_rtl::Exit(error_code);
			return error_code;
		}
		stream << std::string(buffer, read);
	} while (read);
	
	std::istream_iterator<std::string> begin(stream);
	std::istream_iterator<std::string> end;

	std::vector<std::string> elements(begin, end);

	std::string wordcount = std::to_string(elements.size());
	std::string output = wordcount;

	kiv_os_rtl::Write_File(std_out, output.c_str(), output.length(), written);
	kiv_os_rtl::Write_File(std_out, linebreak.c_str(), linebreak.length(), written);

	kiv_os_rtl::Exit(0);
	return 0;
}
