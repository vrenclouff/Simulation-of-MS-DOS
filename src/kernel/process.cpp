#include "kernel.h"
#include <Windows.h>
#include <string>

void Clone(kiv_hal::TRegisters &regs) {
	// TODO clone process

	char* funcname = reinterpret_cast<char*>(regs.rdx.r);
	Call_User_Function(funcname, regs);
}

void Exit(kiv_hal::TRegisters &regs) {
	// TODO
}

void Read_Exit_Code(kiv_hal::TRegisters &regs) {
	// TODO
}

void Register_Signal_Handler(kiv_hal::TRegisters &regs) {
	// TODO
}

void Shutdown(kiv_hal::TRegisters &regs) {
	// TODO
}

void Wait_For(kiv_hal::TRegisters &regs) {
	// TODO
}

void Handle_Process(kiv_hal::TRegisters &regs) {

	switch (static_cast<kiv_os::NOS_Process>(regs.rax.l)) {

		case kiv_os::NOS_Process::Clone:
			Clone(regs);
			break;
		
		case kiv_os::NOS_Process::Exit:
			Exit(regs);
			break;

		case kiv_os::NOS_Process::Read_Exit_Code:
			Read_Exit_Code(regs);
			break;

		case kiv_os::NOS_Process::Register_Signal_Handler:
			Register_Signal_Handler(regs);
			break;

		case kiv_os::NOS_Process::Shutdown:
			Shutdown(regs);
			break;

		case kiv_os::NOS_Process::Wait_For:
			Wait_For(regs);
			break;
	}

}
