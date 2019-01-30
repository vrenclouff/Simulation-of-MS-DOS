#pragma once

#include "kernel.h"
#include <Windows.h>

#include "drive.h"
#include "io.h"
#include "process.h"
#include "common.h"

// @author: template & Lukas Cerny & Petr Volf

HMODULE User_Programs;
Process_Manager *process_manager = new Process_Manager();

void Initialize_Kernel() {
	User_Programs = LoadLibraryW(L"user.dll");
}

void Shutdown_Kernel() {
	delete process_manager;
	FreeLibrary(User_Programs);
}

void __stdcall Sys_Call(kiv_hal::TRegisters &regs) {

	switch (static_cast<kiv_os::NOS_Service_Major>(regs.rax.h)) {
	
		case kiv_os::NOS_Service_Major::File_System:		
			Handle_IO(regs);
			break;
		case kiv_os::NOS_Service_Major::Process:
			process_manager->sys_call(regs);
			break;
	}

}

void __stdcall Bootstrap_Loader(kiv_hal::TRegisters &context) {
	Initialize_Kernel();
	kiv_hal::Set_Interrupt_Handler(kiv_os::System_Int_Number, Sys_Call);

	char drive_name = 67;	// C
	kiv_hal::TRegisters regs;
	for (regs.rdx.l = 0; ; regs.rdx.l++) {

		kiv_hal::TDrive_Parameters params;		
		regs.rax.h = static_cast<uint8_t>(kiv_hal::NDisk_IO::Drive_Parameters);
		regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&params);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		if (!regs.flags.carry) {
			std::vector<unsigned char> arr(params.bytes_per_sector);
			kiv_hal::TDisk_Address_Packet dap;
			dap.sectors = static_cast<void*>(arr.data());
			dap.count = 1;
			dap.lba_index = 0;
			regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);

			// Loading the first sector (Boot Block)
			regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

			// Check if the driver is formatted
			if (!kiv_fs::is_formatted(arr.data())) {
				kiv_fs::format_disk(kiv_fs::FAT_Version::FAT16, arr.data(), params);

				regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
				kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			}

			// Resave boot block to struct
			kiv_fs::FATBoot_Block boot_block;
			kiv_fs::boot_block(boot_block, params.bytes_per_sector, arr.data());

			char volume[2] = { drive_name++, ':'};
			drive::save(std::string(volume, sizeof(volume)), regs.rdx.l, boot_block);
		}

		if (regs.rdx.l == 255) break;
	}

	const auto initProgram = "shell";
	const auto initProgramArgs = "";
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(initProgram);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(initProgramArgs);
	const auto std_handle = Register_STD();
	const uint16_t stdin_handle = std_handle.in;
	const uint16_t stdout_handle = std_handle.out;
	regs.rbx.e = (stdin_handle << 16) | stdout_handle;

	process_manager->create_process(regs, true);
	kiv_os::THandle shell_handle = static_cast<kiv_os::THandle>(regs.rax.x);
	// wait for shell to exit (syscall Wait_For)
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Wait_For);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&shell_handle);
	regs.rcx.r = 1;
	process_manager->handle_wait_for(regs);

	regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_File_System::Close_Handle);
	regs.rdx.x = std_handle.in;
	Handle_IO(regs);

	regs.rdx.x = std_handle.out;
	Handle_IO(regs);

	Shutdown_Kernel();
}

void Call_User_Function(char* funcname, kiv_hal::TRegisters regs) {
	kiv_os::TThread_Proc to_call = (kiv_os::TThread_Proc)GetProcAddress(User_Programs, funcname);
	if (to_call) {
		to_call(regs);
	}
}
