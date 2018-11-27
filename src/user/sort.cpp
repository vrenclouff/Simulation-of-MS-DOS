#include "sort.h"
#include "rtl.h"
#include "error.h"

#include <vector>
#include <iterator>
#include <sstream>
#include <algorithm>


size_t __stdcall sort(const kiv_hal::TRegisters &regs) {

	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);

	char buffer[1024];
	size_t read, written;
	std::stringstream ss;
	do {
		if (!kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read)) {
			const kiv_os::NOS_Error error = kiv_os_rtl::Last_Error;
			const auto error_msg = Error_Message(error);
			kiv_os_rtl::Write_File(std_out, error_msg.data(), error_msg.length(), written);
			const auto error_code = static_cast<uint16_t>(error);
			kiv_os_rtl::Exit(error_code);
			return error_code;
		}
		ss << std::string(buffer, read);
	} while (read);


	std::vector<std::string> elements;
	std::string line;
	while (std::getline(ss, line)) {
		elements.push_back(line);
	}

	std::sort(elements.begin(), elements.end());

	for (std::string item : elements) {
		kiv_os_rtl::Write_File(std_out, item.c_str(), item.length(), written);
		kiv_os_rtl::Write_File(std_out, "\n", 1, written);
	}

	kiv_os_rtl::Exit(0);
	return 0;
}
