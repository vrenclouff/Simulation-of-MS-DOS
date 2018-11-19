#include "process_manager.h"
#include <iostream>
#include <vector>
#include "common.h"
#include <sstream>
#include <string>
#include <iostream>

#include "io_manager.h"

// ---PRIVATE METHODS---

Process* ProcessManager::_getRunningProcess()
{
	size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
	// check if thread is main thread
	if (processes.find(thread_id) != processes.end())
	{
		Process* ret = processes[thread_id];
		return ret;
	}
	return getProcessByTid(thread_id);
}

Thread* ProcessManager::getRunningThread()
{
	size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
	return getThreadByTid(thread_id);
}

Thread* ProcessManager::getThreadByTid(size_t tid)
{
	for (auto const& processes_item : processes)
	{
		Process* process = processes_item.second;
		for (auto const& threads_item : process->threads)
		{
			Thread* thread = threads_item.second;
			if (thread->tid == tid)
			{
				return thread;
			}
		}
	}
	return nullptr;
}

Process* ProcessManager::getProcessByTid(size_t tid)
{
	for (auto const& processes_item : processes)
	{
		Process* process = processes_item.second;
		for (auto const& threads_item : process->threads)
		{
			Thread* thread = threads_item.second;
			if (thread->tid == tid)
			{
				return process;
			}
		}
	}
	return nullptr;
}

void ProcessManager::removeHandle(kiv_os::THandle handle)
{
	handles.erase(handle);
}

void ProcessManager::removeProcess(kiv_os::THandle handle)
{
	size_t pid = handles[handle];
	Process* target_process = processes[pid];
	processes.erase(pid);
	handles.erase(handle);
	delete target_process;
}

// ---PUBLIC METHODS---

Process* ProcessManager::getRunningProcess()
{
	std::lock_guard<std::mutex> lock(mtx);
	return _getRunningProcess();
}

void ProcessManager::SysCall(kiv_hal::TRegisters &regs)
{
	switch (static_cast<kiv_os::NOS_Process>(regs.rax.l))
	{
		case kiv_os::NOS_Process::Clone:
			handleClone(regs);
			break;
		case kiv_os::NOS_Process::Wait_For:
			handleWaitFor(regs);
			break;
		case kiv_os::NOS_Process::Exit:
			handleExit(regs);
			break;
		case kiv_os::NOS_Process::Read_Exit_Code:
			handleReadExitCode(regs);
			break;
	}
}

void ProcessManager::handleClone(kiv_hal::TRegisters &regs)
{
	switch (static_cast<kiv_os::NClone>(regs.rcx.r))
	{
	case kiv_os::NClone::Create_Process:
		createProcess(regs);
		break;
	case kiv_os::NClone::Create_Thread:
		createThread(regs);
		break;
	}
}

void ProcessManager::createProcess(kiv_hal::TRegisters &regs, bool first_process)
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
	kiv_os::THandle stdout_handle = regs.rbx.e & 0x00ff;
	std::string dir;

	if (!first_process)
	{
		Process* currentProcess = _getRunningProcess();
		parent_pid = currentProcess->pid;
		parent_handle = currentProcess->handle;
		dir = currentProcess->working_dir;
	}
	else {
		dir = io::main_drive().append("\\");
	}

	Process* newProcess = new Process(funcNameStr, parent_pid);
	newProcess->working_dir = dir;
	kiv_hal::TRegisters child_context{ 0 };
	child_context.rdi.r = regs.rdi.r; // function args
	child_context.rax.x = stdin_handle; // stdin
	child_context.rbx.x = stdout_handle; // stdout

	newProcess->startThread(child_context, programAddress);
	processes[newProcess->pid] = newProcess;
	handles[++last_handle] = newProcess->pid;
	newProcess->handle = last_handle;
	newProcess->parent_handle = parent_handle;

	// Save child pid to parent ax
	regs.rax.x = static_cast<kiv_os::THandle>(last_handle);
}

void ProcessManager::createThread(kiv_hal::TRegisters &regs)
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

	Process* currentProcess = _getRunningProcess();

	kiv_hal::TRegisters child_context{ 0 };
	child_context.rdi.r = regs.rdi.r; // pointer to shared data
	child_context.rax.x = stdin_handle; // stdin
	child_context.rbx.x = stdout_handle; // stdout
	 
	handles[++last_handle] = currentProcess->startThread(child_context, programAddress);

	// Save child pid to parent ax
	regs.rax.x = static_cast<kiv_os::THandle>(last_handle);
}

void ProcessManager::handleWaitFor(kiv_hal::TRegisters &regs)
{
	// RDX = array of handles
	// RCX = array size
	regs.flags.carry = 0;
	kiv_os::THandle *registered_handles = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r);
	size_t handles_size = static_cast<size_t>(regs.rcx.r);
	std::vector<Thread*> waiting_for(handles_size);

	std::unique_lock<std::mutex> lk(Thread::endMtx);
	mtx.lock();
	for (int i = 0; i < handles_size; i++)
	{
		if (handles.count(registered_handles[i]) == 0)
		{
			regs.flags.carry = 1;
			regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
			lk.unlock();
			mtx.unlock();
			return;
		}
		size_t tid = handles[registered_handles[i]];
		waiting_for[i] = getThreadByTid(tid);
		if (waiting_for[i]->state == ThreadState::stopped)
		{
			regs.rax.r = static_cast<uint64_t>(registered_handles[i]);
			lk.unlock();
			mtx.unlock();
			return;
		}
	}
	mtx.unlock();

	while (true)
	{
		Thread::endCond.wait(lk);
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

void ProcessManager::handleExit(kiv_hal::TRegisters &regs)
{
	// CX = exit code
	mtx.lock();
	uint16_t exitCode = static_cast<uint16_t>(regs.rcx.x);
	Process* thisProcess = _getRunningProcess();
	size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
	mtx.unlock();
	thisProcess->stopThread(exitCode, thread_id);
}

void ProcessManager::handleReadExitCode(kiv_hal::TRegisters &regs)
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
	Thread* target_thread = getThreadByTid(target_tid);
	if (target_thread->state != ThreadState::stopped)
	{
		regs.flags.carry = 1;
		regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		return;
	}
	uint16_t exit_code = target_thread->exitCode;
	regs.rcx.x = exit_code;

	Process* target_process = getProcessByTid(target_tid);
	target_process->cleanThread(target_tid);

	if (target_process->state == ProcessState::stopped)
	{
		removeProcess(target_handle);
	}
	else
	{
		removeHandle(target_handle);
	}
}

std::string ProcessManager::getProcessTable()
{
	std::lock_guard<std::mutex> lock(mtx);
	std::ostringstream result;
	result << "PID\tPPID\tSTATUS\tCOMMAND" << std::endl;
	for (auto const& process_entry : processes)
	{
		Process* process = process_entry.second;
		std::string process_state;
		switch (process->state)
		{
		case ProcessState::prepared:
			process_state = "prepared";
			break;
		case ProcessState::running:
			process_state = "running";
			break;
		case ProcessState::stopped:
			process_state = "stopped";
			break;
		default:
			process_state = "unknown";
			break;
		}
		result << process->handle << "\t" << process->parent_handle << "\t" << process_state << "\t" << process->userfunc_name << std::endl;
	}
	return result.str();
}