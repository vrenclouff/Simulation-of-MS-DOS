#include "shell.h"
#include "rtl.h"
#include "parser.h"

size_t __stdcall shell(const kiv_hal::TRegisters &regs) {

	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	const size_t buffer_size = 256;
	char buffer[buffer_size];
	size_t read_counter = 0, write_counter = 0;

	char prompt[100];
	const char* exit = "exit";
	do {
		kiv_os_rtl::Get_Working_Dir(prompt, sizeof(prompt), read_counter);
		kiv_os_rtl::Write_File(std_out, prompt, read_counter, write_counter);
		kiv_os_rtl::Write_File(std_out, ">", 1, write_counter);

		if (kiv_os_rtl::Read_File(std_in, buffer, buffer_size, read_counter)) {
			kiv_os_rtl::Write_File(std_out, "\n", 1, write_counter);

			if (read_counter == 0) continue;

			if (read_counter == buffer_size) {
				read_counter--;
			}
			buffer[read_counter] = 0;	//udelame z precteneho vstup null-terminated retezec

			if (strcmp(buffer, exit) == 0) break;

			parse(buffer, std_in, std_out, read_counter);
		}
		else {
			break;	//EOF
		}
	} while (strcmp(buffer, exit) != 0);

	bool exitcode = 0;
	kiv_os_rtl::Exit(exitcode);
	
	return exitcode;	
}
