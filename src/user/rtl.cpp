#include "rtl.h"

#include <string>
#include <iterator>
#include <iostream>
#include <sstream>
#include <vector>

std::atomic<kiv_os::NOS_Error> kiv_os_rtl::Last_Error;

kiv_hal::TRegisters Prepare_SysCall_Context(kiv_os::NOS_Service_Major major, uint8_t minor) {
	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<uint8_t>(major);
	regs.rax.l = minor;
	return regs;
}


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

bool kiv_os_rtl::Clone(char* args) {

	char* arguments;
	char* tofunc = strtok_s(args, " ", &arguments);

	if (tofunc == NULL)
	{
		return NULL;
	}

	size_t origsize = strlen(tofunc);
	size_t size = origsize + sizeof(char);
	char* function = (char*)malloc(size);
	strncpy_s(function, size, tofunc, size);

	// TODO protoze api/user.def - asi vyresit lip
	if (!strcmp(function, "find")) {
		function = "wc";
	}
	else if (!strcmp(function, "tasklist")) {
		function = "ps";
	}
	else if (!strcmp(function, "wc") || ! strcmp(function, "ps")) {
		function = "";
	}

	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Clone));
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(function);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(arguments);
	// set standard stdin / stdout (TODO different values on pipe)
	uint16_t stdin_handle = 0;
	uint16_t stdout_handle = 1;
	regs.rbx.e = (stdin_handle << 16) | stdout_handle;
	regs.rcx.r = static_cast<uint64_t>(kiv_os::NClone::Create_Process);

	const bool result = kiv_os::Sys_Call(regs);
	return result;
}

bool kiv_os_rtl::Shutdown() {
	kiv_hal::TRegisters regs = Prepare_SysCall_Context(kiv_os::NOS_Service_Major::Process, static_cast<uint8_t>(kiv_os::NOS_Process::Shutdown));
	const bool result = kiv_os::Sys_Call(regs);
	return result;
}
