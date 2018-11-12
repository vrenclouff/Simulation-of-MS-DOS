#pragma once

#include "kernel.h"
#include <Windows.h>

#include "io.h"
#include "fat.h"
#include "fat_tools.h"
#include "fat_file.h"
#include "process.h"
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

	char drive_name = 67;	// C
	kiv_hal::TRegisters regs;
	for (regs.rdx.l = 0; ; regs.rdx.l++) {

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

			auto hard_drive = [&regs](unsigned char* data, uint16_t sector, kiv_hal::NDisk_IO operation) {
				kiv_hal::TDisk_Address_Packet dap;
				dap.sectors = static_cast<void*>(data);
				dap.count = 1;
				dap.lba_index = sector;
				regs.rax.h = static_cast<decltype(regs.rax.h)>(operation);
				regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
				kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			};


			// Loading the first sector (Boot Block)
			std::vector<unsigned char> arr(params.bytes_per_sector);
			hard_drive(arr.data(), 0, kiv_hal::NDisk_IO::Read_Sectors);

			// Check if the driver is formatted
			if (!kiv_fs::is_formatted(arr.data())) {
				kiv_fs::format_disk(kiv_fs::FAT_Version::FAT16, arr.data(), params);
				hard_drive(arr.data(), 0, kiv_hal::NDisk_IO::Write_Sectors);
			}

			// Resave boot block to struct
			kiv_fs::FATBoot_Block boot_block;
			kiv_fs::boot_block(boot_block, params.bytes_per_sector, arr.data());

			char volume[2] = { drive_name++, ':'};
			std::string drive_volume = std::string(volume, sizeof(volume));
			if (io::register_drive(drive_volume, regs.rdx.l, boot_block)) {
				// TODO exception -> can't register disk drive
			}

			{
				drive_volume.append("\\");
				kiv_hal::TRegisters regs;
				regs.rax.l = static_cast<uint8_t>(kiv_os::NOS_File_System::Set_Working_Dir);
				regs.rdx.r = reinterpret_cast<decltype(regs.rdi.r)>(drive_volume.c_str());
				regs.rcx.r = drive_volume.size();
				Handle_IO(regs);
			}
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

	process_manager->createProcess(regs, true);
	kiv_os::THandle shell_handle = static_cast<kiv_os::THandle>(regs.rax.x);
	// wait for shell to exit (syscall Wait_For)
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_os::NOS_Service_Major::Process);
	regs.rax.l = static_cast<decltype(regs.rax.l)>(kiv_os::NOS_Process::Wait_For);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&shell_handle);
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
