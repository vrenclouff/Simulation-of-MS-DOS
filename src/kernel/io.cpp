#include "io.h"

#include "handles.h"
#include "common.h"
#include "fs_tools.h"
#include "io_manager.h"

#include <functional>
#include <vector>

std::mutex Pipe_Guard;
std::vector<kiv_os::THandle> pipes;

void Set_Error(const bool failed, kiv_hal::TRegisters &regs, kiv_os::NOS_Error error) {
	if (failed) {
		regs.flags.carry = true;
		regs.rax.x = static_cast<decltype(regs.rax.x)>(error);
	}
	else {
		regs.flags.carry = false;
	}
}

STDHandle Register_STD() {
	const auto  in = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_Keyboard()));
	const auto out = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_VGA()));
	return { in, out };
}

bool is_pipe(const kiv_os::THandle handle) {
	std::lock_guard<std::mutex> locker(Pipe_Guard);

	return std::find(pipes.begin(), pipes.end(), handle) != pipes.end();
}

void add_pipe(const kiv_os::THandle handle) {
	std::lock_guard<std::mutex> locker(Pipe_Guard);

	pipes.push_back(handle);
}

void Open_File(kiv_hal::TRegisters &regs) {
	const auto path = reinterpret_cast<char*>(regs.rdx.r);
	const auto fm = static_cast<kiv_os::NOpen_File>(regs.rcx.l);
	const auto attributes = static_cast<kiv_os::NFile_Attributes>(regs.rdi.i);

	kiv_os::NOS_Error error = kiv_os::NOS_Error::Success;
	const auto source = io::Open_File(path, fm, attributes, error);

	Set_Error(!source, regs, error);
	if (regs.flags.carry) return;

	regs.rax.x = Convert_Native_Handle(static_cast<HANDLE>(source));;
}

void Read_File(kiv_hal::TRegisters &regs) {
	const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
	auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto buffer_size = regs.rcx.r;

	kiv_os::NOS_Error error = kiv_os::NOS_Error::Success;
	regs.rax.r = source->read(buffer, buffer_size, error);

	Set_Error(error != kiv_os::NOS_Error::Success , regs, error);
}

void Write_File(kiv_hal::TRegisters &regs) {
	const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
	auto buffer = reinterpret_cast<char*>(regs.rdi.r);
	const auto buffer_size = regs.rcx.r;

	kiv_os::NOS_Error error = kiv_os::NOS_Error::Success;
	regs.rax.r = source->write(buffer, buffer_size, error);

	Set_Error(regs.rax.r == 0, regs, error);
}

void Delete_File(kiv_hal::TRegisters &regs) {
	auto path = std::string(reinterpret_cast<char*>(regs.rdx.r));
	const auto process = process_manager->getRunningProcess();

	kiv_os::NOS_Error error = kiv_os::NOS_Error::Success;
	const auto success = io::Remove_File(fs_tool::to_absolute_path(process->working_dir, path), error);

	Set_Error(success, regs, error);
}

void Close_Handle(kiv_hal::TRegisters &regs) {
	const auto handle = regs.rdx.x;
	const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(handle));

	if (is_pipe(handle)) {
		source->close();
	}
	const auto success = Remove_Handle(handle);

	Set_Error(!success, regs, kiv_os::NOS_Error::Unknown_Error);

	if (success) {delete source;}
}

void Get_Working_Dir(kiv_hal::TRegisters &regs) {
	auto path_buffer = reinterpret_cast<char*>(regs.rdx.r);
	const auto buffer_size = regs.rcx.r;
	const auto process = process_manager->getRunningProcess();
	const auto working_dir = std::string(process->working_dir);
	const size_t size = buffer_size <= working_dir.size() ? buffer_size : working_dir.size();
	std::copy(&working_dir[0], &working_dir[0] + size, path_buffer);

	regs.rax.r = size;
	regs.flags.carry = 0;
}

void Set_Working_Dir(kiv_hal::TRegisters &regs) {
	const auto path = std::string(reinterpret_cast<char*>(regs.rdx.r));
	const auto process = process_manager->getRunningProcess();
	const auto full_path = fs_tool::to_absolute_path(process->working_dir, path);
	const auto is_exist = io::is_exist_dir(full_path);

	Set_Error(!is_exist, regs, kiv_os::NOS_Error::File_Not_Found);
	if (regs.flags.carry) return;

	process->working_dir = full_path;
}

void Seek(kiv_hal::TRegisters &regs) {
	const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
	// set seek to source
}

void Create_Pipe(kiv_hal::TRegisters &regs) {
	kiv_os::THandle* handle_ptr = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r);
	std::shared_ptr<Circular_buffer> pipe = std::make_shared<Circular_buffer>();
	auto pipe_to_write = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_Pipe(pipe, Permission::Write)));
	add_pipe(pipe_to_write);
	*(handle_ptr) = pipe_to_write;
	*(handle_ptr + 1) = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_Pipe(pipe, Permission::Read)));

	regs.flags.carry = 0;
}



void Handle_IO(kiv_hal::TRegisters &regs) {
	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {
		case kiv_os::NOS_File_System::Open_File:		Open_File(regs);		break;
		case kiv_os::NOS_File_System::Read_File:		Read_File(regs);		break;
		case kiv_os::NOS_File_System::Write_File:		Write_File(regs);		break;
		case kiv_os::NOS_File_System::Delete_File:		Delete_File(regs);		break;
		case kiv_os::NOS_File_System::Close_Handle:		Close_Handle(regs);		break;
		case kiv_os::NOS_File_System::Get_Working_Dir:	Get_Working_Dir(regs);	break;
		case kiv_os::NOS_File_System::Set_Working_Dir:	Set_Working_Dir(regs);	break;
		case kiv_os::NOS_File_System::Create_Pipe:		Create_Pipe(regs);		break;
		case kiv_os::NOS_File_System::Seek:				Seek(regs);				break;
	}
}