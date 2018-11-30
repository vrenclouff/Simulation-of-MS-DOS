#include "rtl.h"

#include <string>
#include <iterator>
#include <iostream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <array>

std::atomic<kiv_os::NOS_Error> kiv_os_rtl::Last_Error;

kiv_hal::TRegisters Prepare_SysCall_Context(kiv_os::NOS_Service_Major major, uint8_t minor) {
	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(major);
	regs.rax.l = minor;
	regs.flags.carry = 0;
	return regs;
}

// FILE SYSTEM

bool kiv_os_rtl::Read_File(const kiv_os::THandle file_handle, char* const buffer, const size_t buffer_size, size_t &read) {
	kiv_hal::TRegisters regs =  Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Read_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;	

	const bool result = kiv_os::Sys_Call(regs);
	read = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Write_File));
	regs.rdx.x = static_cast<decltype(regs.rdx.x)>(file_handle);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(buffer);
	regs.rcx.r = buffer_size;

	const bool result = kiv_os::Sys_Call(regs);
	written = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Get_Working_Dir(const char *buffer, const size_t buffer_size, size_t &read) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Get_Working_Dir));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(buffer);
	regs.rcx.r = buffer_size;
	const bool result = kiv_os::Sys_Call(regs);
	read = regs.rax.r;
	return result;
}

bool kiv_os_rtl::Set_Working_Dir(const char *dir) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Set_Working_Dir));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(dir);

	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

bool kiv_os_rtl::Open_File(const char *buffer, const size_t buffer_size, kiv_os::THandle &file_handle, const bool exist, const kiv_os::NFile_Attributes attributes) {

	std::string str_path = std::string(buffer, buffer_size);
	if (std::filesystem::u8path(str_path).is_relative()) {
		char absolute_path[100];
		size_t counter;
		Get_Working_Dir(absolute_path, sizeof(absolute_path), counter);
		auto path = std::filesystem::u8path(std::string(absolute_path, counter));
		path /= str_path;
		str_path = path.lexically_normal().string();
	}

	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Open_File));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(str_path.data());
	regs.rcx.l = exist ? static_cast<decltype(regs.rcx.l)>(kiv_os::NOpen_File::fmOpen_Always) : 0;
	const auto read_only = static_cast<uint8_t>(kiv_os::NFile_Attributes::Read_Only);
	regs.rdi.i = static_cast<decltype(regs.rdi.i)>(attributes);

	const bool result = kiv_os::Sys_Call(regs);
	file_handle = regs.rax.x;
	return result;	
}

bool kiv_os_rtl::Delete_File(const char* filename) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Delete_File));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(filename);
	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

bool kiv_os_rtl::Close_Handle(const kiv_os::THandle file_handle) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Close_Handle));
	regs.rdx.x = file_handle;
	return kiv_os::Sys_Call(regs);
}

bool kiv_os_rtl::Create_Pipe(kiv_os::THandle handlers[]) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::File_System, static_cast<uint8_t>(kiv_os::NOS_File_System::Create_Pipe));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(handlers);

	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

// PROCESS

bool kiv_os_rtl::Clone(kiv_os::THandle& pid, const char* function, const char* arguments, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle) {

	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Clone));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(function);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(arguments);

	regs.rbx.e = (stdin_handle << 16) | stdout_handle;
	regs.rcx.r = static_cast<uint64_t>(kiv_os::NClone::Create_Process);

	const bool result = kiv_os::Sys_Call(regs);
	pid = static_cast<kiv_os::THandle>(regs.rax.r);
	return result;
}

bool kiv_os_rtl::Wait_For(kiv_os::THandle handle) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(&handle);
	//regs.rcx.r = static_cast<decltype(regs.rdx.r)>(handlers_count);
	regs.rcx.r = 1;

	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

bool kiv_os_rtl::Exit(uint16_t exitcode) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Exit));
	regs.rcx.r = static_cast<decltype(regs.rcx.r)>(exitcode);

	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

std::uint8_t kiv_os_rtl::Read_Exit_Code(kiv_os::THandle handle) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Read_Exit_Code));
	regs.rdx.r = static_cast<decltype(regs.rdx.r)>(handle);

	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

bool kiv_os_rtl::Shutdown() {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Shutdown));
	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

bool kiv_os_rtl::Register_Signal_Handler(kiv_os::NSignal_Id signal, kiv_os::TThread_Proc handler) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Register_Signal_Handler));
	regs.rcx.r = static_cast<uint64_t>(signal);
	regs.rdx.r = reinterpret_cast<uint64_t>(handler);
	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

kiv_os::THandle kiv_os_rtl::Create_Thread(void* function, void* data, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle) {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Clone));

	regs.rcx.r = static_cast<uint64_t>(kiv_os::NClone::Create_Thread);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(function);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(data);
	regs.rbx.e = (stdin_handle << 16) | stdout_handle;

	const bool result = kiv_os::Sys_Call(regs);
	return static_cast<kiv_os::THandle>(regs.rax.x);
}
