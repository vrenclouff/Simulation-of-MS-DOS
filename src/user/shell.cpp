#include "shell.h"
#include "rtl.h"
#include "commands.h"

#define EXIT	"exit"

std::atomic_bool is_sigterm_shell = false;
std::atomic_bool shell_echo = true;
void SIGTERM_Handler_shell(const kiv_hal::TRegisters &regs) {
	is_sigterm_shell = true;
}

size_t __stdcall shell(const kiv_hal::TRegisters &regs) {

	kiv_os_rtl::Register_Signal_Handler(kiv_os::NSignal_Id::Terminate, reinterpret_cast<kiv_os::TThread_Proc>(SIGTERM_Handler_shell));

	const auto std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const auto std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	char buffer[256];
	size_t read, written;

	do {
		if (shell_echo) {
			char prompt[100];
			kiv_os_rtl::Get_Working_Dir(prompt, sizeof(prompt), read);
			kiv_os_rtl::Write_File(std_out, prompt, read, written);
			kiv_os_rtl::Write_File(std_out, ">", 1, written);
		}

		if (kiv_os_rtl::Read_File(std_in, buffer, sizeof buffer, read)) {
			if (read == 0) break;
			if (read == 1) continue;

			buffer[--read] = 0;

			if (strcmp(buffer, EXIT) == 0) break;
			
			cmd::Error error;
			if (!parse_cmd(std::string(buffer, read), std_in, std_out, error)) {
				kiv_os_rtl::Write_File(std_out, error.what.data(), error.what.size(), written);
			}

			if (is_sigterm_shell) break;
		}
		else {
			break;	//EOF
		}
	} while (strcmp(buffer, EXIT) != 0);

	kiv_os_rtl::Exit(0);
	return 0;	
}
