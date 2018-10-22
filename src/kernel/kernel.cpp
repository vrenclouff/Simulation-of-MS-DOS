#pragma once

#include "kernel.h"
#include "io.h"
#include "process.h"
#include <Windows.h>
#include "common.h"

HMODULE User_Programs;
ProcessManager *process_manager = new ProcessManager();

void Initialize_Kernel() {
	User_Programs = LoadLibraryW(L"user.dll");
}

void Shutdown_Kernel() {
	FreeLibrary(User_Programs);
}

void __stdcall Sys_Call(kiv_hal::TRegisters &regs) {
	switch (static_cast<kiv_os::NOS_Service_Major>(regs.rax.h)) {
	
		case kiv_os::NOS_Service_Major::File_System:		
			Handle_IO(regs);
			break;
		case kiv_os::NOS_Service_Major::Process:
			process_manager->SysCall(regs);
			break;
	}

}

void __stdcall Bootstrap_Loader(kiv_hal::TRegisters &context) {
	Initialize_Kernel();
	kiv_hal::Set_Interrupt_Handler(kiv_os::System_Int_Number, Sys_Call);

	//v ramci ukazky jeste vypiseme dostupne disky
	kiv_hal::TRegisters regs;
	for (regs.rdx.l = 0; ; regs.rdx.l++) {

		// najit prvni dostupny disk, ze ktereho lze "bootovat"
		// v tomto pripade najit ten jediny co tam je a vytvorit nad nim FS

		// disk je pouze virtualni v pameti, po kazdem spusteni se vytvori FS
		// prvni sektor je MBR (master boot record), kde budou informace o FS viz FAT

		// s diskem se pracuji pomoci handleru, kde prvni parametr je obsluzna rutina a druhy registry obsahujici data

		kiv_hal::TDrive_Parameters params;
		regs.rax.h = static_cast<uint8_t>(kiv_hal::NDisk_IO::Drive_Parameters);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&params);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		if (!regs.flags.carry) {
			auto print_str = [](const char* str) {
				kiv_hal::TRegisters regs;
				regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_File_System::Write_File);
				regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(str);
				regs.rcx.r = strlen(str);
				Handle_IO(regs);
			};

			const char dec_2_hex[16] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
			char hexa[3];
			hexa[0] = dec_2_hex[regs.rdx.l >> 4];
			hexa[1] = dec_2_hex[regs.rdx.l & 0xf];
			hexa[2] = 0;

			print_str("Nalezen disk: 0x");
			print_str(hexa);
			print_str("\n");

		}

		if (regs.rdx.l == 255) break;
	}


	// inicializace thread a process manager
	// spustit pod managerem shell

	//spustime shell - v realnem OS bychom ovsem spousteli login
	char* initProgram = "shell";
	char* initProgramArgs = "";
	regs.rdx.r = reinterpret_cast<uint64_t>(initProgram);
	regs.rdi.r = reinterpret_cast<uint64_t>(initProgramArgs);
	uint16_t stdin_handle = 0;
	uint16_t stdout_handle = 1;
	regs.rbx.e = (stdin_handle << 16) | stdout_handle;

	process_manager->createProcess(regs, true);
	kiv_os::THandle shell_handle = static_cast<kiv_os::THandle>(regs.rax.x);
	// wait for shell to exit (syscall Wait_For)
	regs.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For);
	regs.rdx.r = reinterpret_cast<uint64_t>(&shell_handle);
	regs.rcx.r = 1;
	process_manager->handleWaitFor(regs);
	Shutdown_Kernel();
}


void Set_Error(const bool failed, kiv_hal::TRegisters &regs) {
	if (failed) {
		regs.flags.carry = true;
		regs.rax.r = GetLastError();
	}
	else
		regs.flags.carry = false;
}

void Call_User_Function(char* funcname, kiv_hal::TRegisters regs) {
	kiv_os::TThread_Proc to_call = (kiv_os::TThread_Proc)GetProcAddress(User_Programs, funcname);
	if (to_call) {
		to_call(regs);
	}
}
