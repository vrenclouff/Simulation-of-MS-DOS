#include "process_manager.h"
#include <iostream>
#include <vector>
#include "common.h"
#include <sstream>
#include <string>
#include <iostream>

#include "drive.h"


// ---PRIVATE METHODS---

size_t Process_Manager::_get_running_tid()
{
	size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
	return thread_id;
}

Process* Process_Manager::_get_running_process()
{
	size_t thread_id = _get_running_tid();
	// check if thread is main thread
	if (processes.find(thread_id) != processes.end())
	{
		Process* ret = processes[thread_id].get();
		return ret;
	}
	return get_process_by_tid(thread_id);
}

Thread* Process_Manager::_get_running_thread()
{
	size_t thread_id = _get_running_tid();
	return get_thread_by_tid(thread_id);
}

Thread* Process_Manager::get_thread_by_tid(size_t tid)
{
	for (auto const& processes_item : processes)
	{
		Process* process = processes_item.second.get();
		for (auto const& threads_item : process->threads)
		{
			Thread* thread = threads_item.second.get();
			if (thread->tid == tid)
			{
				return thread;
			}
		}
	}
	return nullptr;
}

Process* Process_Manager::get_process_by_tid(size_t tid)
{
	for (auto const& processes_item : processes)
	{
		Process* process = processes_item.second.get();
		for (auto const& threads_item : process->threads)
		{
			Thread* thread = threads_item.second.get();
			if (thread->tid == tid)
			{
				return process;
			}
		}
	}
	return nullptr;
}

void Process_Manager::remove_handle(kiv_os::THandle handle)
{
	handles.erase(handle);
}

void Process_Manager::remove_process(kiv_os::THandle handle)
{
	size_t pid = handles[handle];
	Process* target_process = processes[pid].get();
	processes.erase(pid);
	handles.erase(handle);
}

// ---PUBLIC METHODS---

Process* Process_Manager::get_running_process()
{
	std::lock_guard<std::mutex> lock(mtx);
	return _get_running_process();
}

Thread* Process_Manager::get_running_thread()
{
	std::lock_guard<std::mutex> lock(mtx);
	return _get_running_thread();
}

void Process_Manager::sys_call(kiv_hal::TRegisters &regs)
{
	switch (static_cast<kiv_os::NOS_Process>(regs.rax.l))
	{
		case kiv_os::NOS_Process::Clone:
			handle_clone(regs);
			break;
		case kiv_os::NOS_Process::Wait_For:
			handle_wait_for(regs);
			break;
		case kiv_os::NOS_Process::Exit:
			handle_exit(regs);
			break;
		case kiv_os::NOS_Process::Read_Exit_Code:
			handle_read_exit_code(regs);
			break;
		case kiv_os::NOS_Process::Register_Signal_Handler:
			register_signal_handler(regs);
			break;
		case kiv_os::NOS_Process::Shutdown:
			shutdown(regs);
			break;
	}
}

void Process_Manager::handle_clone(kiv_hal::TRegisters &regs)
{
	switch (static_cast<kiv_os::NClone>(regs.rcx.r))
	{
	case kiv_os::NClone::Create_Process:
		create_process(regs);
		break;
	case kiv_os::NClone::Create_Thread:
		create_thread(regs);
		break;
	}
}

void Process_Manager::create_process(kiv_hal::TRegisters &regs, bool first_process)
{
	std::lock_guard<std::mutex> lock(mtx);
	regs.flags.carry = 0;
	char* funcName = reinterpret_cast<char*>(regs.rdx.r);
	std::string funcNameStr(funcName);
	kiv_os::TThread_Proc programAddress = (kiv_os::TThread_Proc)GetProcAddress(User_Programs, funcName);

	if (!programAddress)
	{
		regs.flags.carry = 1;
		regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		return;
	}

	size_t parent_pid = 0;
	kiv_os::THandle parent_handle = 0;
	kiv_os::THandle stdin_handle = regs.rbx.e >> 16;
	kiv_os::THandle stdout_handle = regs.rbx.e & 0xFFFF;
	std::string dir;

	if (!first_process)
	{
		Process* currentProcess = _get_running_process();
		parent_pid = currentProcess->pid;
		parent_handle = currentProcess->handle;
		dir = currentProcess->working_dir;
	}
	else {
		auto volume = drive::main_volume();
		dir = volume.append("\\");
	}

	std::unique_ptr<Process> newProcess = std::make_unique<Process>(funcNameStr, parent_pid);
	newProcess->working_dir = dir;
	kiv_hal::TRegisters child_context{ 0 };
	child_context.rdi.r = regs.rdi.r; // function args
	child_context.rax.x = stdin_handle; // stdin
	child_context.rbx.x = stdout_handle; // stdout

	newProcess->start_thread(child_context, programAddress);
	handles[++last_handle] = newProcess->pid;
	newProcess->handle = last_handle;
	newProcess->parent_handle = parent_handle;
	size_t temp_pid = newProcess->pid;
	processes.insert(std::pair<size_t, std::unique_ptr<Process>>(temp_pid, std::move(newProcess)));
	//processes[newProcess->pid] = std::monewProcess;

	// Save child pid to parent ax
	regs.rax.x = static_cast<kiv_os::THandle>(last_handle);
}

void Process_Manager::create_thread(kiv_hal::TRegisters &regs)
{	
	std::lock_guard<std::mutex> lock(mtx);
	regs.flags.carry = 0;
	kiv_os::TThread_Proc programAddress = (kiv_os::TThread_Proc) regs.rdx.r;

	if (!programAddress)
	{
		regs.flags.carry = 1;
		regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		return;
	}

	kiv_os::THandle stdin_handle = regs.rbx.e >> 16;
	kiv_os::THandle stdout_handle = regs.rbx.e & 0x00ff;

	Process* currentProcess = _get_running_process();

	kiv_hal::TRegisters child_context{ 0 };
	child_context.rdi.r = regs.rdi.r; // pointer to shared data
	child_context.rax.x = stdin_handle; // stdin
	child_context.rbx.x = stdout_handle; // stdout
	 
	handles[++last_handle] = currentProcess->start_thread(child_context, programAddress);

	// Save child pid to parent ax
	regs.rax.x = static_cast<kiv_os::THandle>(last_handle);
}

void Process_Manager::handle_wait_for(kiv_hal::TRegisters &regs)
{
	// RDX = array of handles
	// RCX = array size
	std::unique_lock<std::mutex> lock(mtx);
	regs.flags.carry = 0;
	kiv_os::THandle *registered_handles = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r);
	size_t handles_size = static_cast<size_t>(regs.rcx.r);
	std::vector<Thread*> waiting_for(handles_size);


	for (int i = 0; i < handles_size; i++)
	{
		if (handles.count(registered_handles[i]) == 0)
		{
			regs.flags.carry = 1;
			regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
			return;
		}
		size_t tid = handles[registered_handles[i]];
		waiting_for[i] = get_thread_by_tid(tid);
		if (waiting_for[i]->state == ThreadState::stopped)
		{
			regs.rax.r = static_cast<uint64_t>(registered_handles[i]);
			return;
		}
	}

	while (true)
	{
		Thread::endCond.wait(lock);
		for (int i = 0; i < handles_size; i++)
		{
			if (waiting_for[i]->state == ThreadState::stopped)
			{
				regs.rax.r = static_cast<uint64_t>(registered_handles[i]);
				return;
			}
		}
	}
}

void Process_Manager::handle_exit(kiv_hal::TRegisters &regs)
{
	// CX = exit code
	uint16_t exitCode;
	size_t thread_id;
	Process* thisProcess;
	{
		std::lock_guard<std::mutex> lock(mtx);
		exitCode = static_cast<uint16_t>(regs.rcx.x);
		thisProcess = _get_running_process();
		thread_id = _get_running_tid();
	}
	thisProcess->stop_thread(exitCode, thread_id);
}

void Process_Manager::handle_read_exit_code(kiv_hal::TRegisters &regs)
{
	// dx = process / thread handle
	// OUT: cx = exitcode
	std::lock_guard<std::mutex> lock(mtx);
	kiv_os::THandle target_handle = static_cast<uint16_t>(regs.rdx.x);
	regs.flags.carry = 0;
	if (handles.count(target_handle) == 0)
	{
		regs.flags.carry = 1;
		regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		return;
	}
	size_t target_tid = handles[target_handle];
	Thread* target_thread = get_thread_by_tid(target_tid);
	if (target_thread->state != ThreadState::stopped)
	{
		regs.flags.carry = 1;
		regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		return;
	}
	uint16_t exit_code = target_thread->exitCode;
	regs.rcx.x = exit_code;

	Process* target_process = get_process_by_tid(target_tid);
	target_process->clean_thread(target_tid);

	if (target_process->state == ProcessState::stopped)
	{
		remove_process(target_handle);
	}
	else
	{
		remove_handle(target_handle);
	}
}

void Process_Manager::register_signal_handler(kiv_hal::TRegisters &regs)
{
	// rcx NSignal_Id
	// rdx: pointer na TThread_Proc, kde pri jeho volani context.rcx bude id signalu (0 = default handler)
	std::lock_guard<std::mutex> lock(mtx);
	regs.flags.carry = 0;
	kiv_os::NSignal_Id signal = static_cast<kiv_os::NSignal_Id>(regs.rcx.r);
	if (signal != kiv_os::NSignal_Id::Terminate)
	{
		regs.flags.carry = 1;
		regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		return;
	}

	kiv_os::TThread_Proc handle;
	if (regs.rdx.r == 0)
	{
		handle = reinterpret_cast<kiv_os::TThread_Proc>(defaultTerminateHandle);
	}
	else
	{
		handle = reinterpret_cast<kiv_os::TThread_Proc>(regs.rdx.r);
	}

	Thread* this_thread = _get_running_thread();
	this_thread->handlers[signal] = handle;
}

void Process_Manager::shutdown(kiv_hal::TRegisters &regs)
{
	// rcx NSignal_Id
	// rdx: pointer na TThread_Proc, kde pri jeho volani context.rcx bude id signalu (0 = default handler)
	std::lock_guard<std::mutex> lock(mtx);

	// send sigterm
	kiv_hal::TRegisters sigterm_regs{ 0 };
	sigterm_regs.rcx.r = static_cast<uint64_t>(kiv_os::NSignal_Id::Terminate);
	for (auto const& process_entry : processes)
	{
		for (auto const& thread_entry : process_entry.second->threads)
		{
			thread_entry.second->handlers[kiv_os::NSignal_Id::Terminate](sigterm_regs);
		}
	}

}

std::string Process_Manager::get_process_table()
{
	std::lock_guard<std::mutex> lock(mtx);
	std::ostringstream result;

	for (auto const& process_entry : processes) {
		const auto process = process_entry.second.get();

		result << process->userfunc_name << "\t";

		switch (process->state) {
			case ProcessState::prepared:	result << "prepared";	break;
			case ProcessState::running:		result << "running";	break;
			case ProcessState::stopped:		result << "stopped";	break;
			default:						result << "unknown";	break;
		}
		result << "\t" << process->handle << "\n";
	}
	return result.str();
}
