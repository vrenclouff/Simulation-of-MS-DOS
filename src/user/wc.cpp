#include "wc.h"
#include "rtl.h"
#include "error.h"

#include <string>
#include <sstream>
#include <iterator>
#include <vector>

size_t __stdcall wc(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

	const auto input = reinterpret_cast<const char*>(regs.rdi.r);

	std::string first, second, arg;

	std::istringstream ss(input);
	auto input_itr = std::istream_iterator<std::string>(ss);

	first = *input_itr++;
	second = *input_itr++;
	for (; input_itr != std::istream_iterator<std::string>(); input_itr++) {
		arg.append(*input_itr);
	}

	size_t written, read;
	
	if (first.compare("/c") != 0 || second.compare("/v\"\"") != 0 || !arg.empty()) {
		const kiv_os::NOS_Error error = kiv_os::NOS_Error::Invalid_Argument;
		const auto error_msg = Error_Message(error);
		const auto error_code = static_cast<uint16_t>(error);

		kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.length(), written);
		kiv_os_rtl::Exit(error_code);
		return error_code;
	}
	
	char buffer[1024];
	std::stringstream stream;
	do {
		if (kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read)) {
			stream << std::string(buffer, read);
		}
		else {
			const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
			const auto error_msg = Error_Message(error);
			const auto error_code = static_cast<uint16_t>(error);

			kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.length(), written);
			kiv_os_rtl::Exit(error_code);
			return error_code;
		}
	} while (read);
	
	std::vector<std::string> elements;
	std::string line;
	while (std::getline(stream, line)) {
		elements.push_back(line);
	}

	const auto nm_of_lines = std::to_string(elements.size()).append("\n");
	kiv_os_rtl::Write_File(std_out, nm_of_lines.data(), nm_of_lines.size(), written);

	kiv_os_rtl::Exit(0);
	return 0;
}
