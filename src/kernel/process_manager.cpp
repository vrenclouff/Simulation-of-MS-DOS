#include "process_manager.h"
#include <iostream>
#include <vector>
#include "common.h"
#include <sstream>
#include <string>
#include <iostream>

Process* ProcessManager::getRunningProcess()
{
	size_t thread_id = std::hash<std::thread::id>()(std::this_thread::get_id());
	return processes[thread_id];
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
	regs.flags.carry = 0;
	char* funcName = reinterpret_cast<char*>(regs.rdx.r);
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

	if (!first_process)
	{
		Process* currentProcess = getRunningProcess();
		parent_pid = currentProcess->pid;
		parent_handle = currentProcess->handle;
	}

	Process* newProcess = new Process(funcName, parent_pid, false);
	kiv_hal::TRegisters child_context{ 0 };
	child_context.rdi.r = regs.rdi.r; // function args
	child_context.rax.x = stdin_handle; // stdin
	child_context.rbx.x = stdout_handle; // stdout

	process_map_mtx.lock();
	newProcess->start(child_context, programAddress);
	processes[newProcess->pid] = newProcess;
	handles[++last_handle] = newProcess->pid;
	newProcess->handle = last_handle;
	newProcess->parent_handle = parent_handle;
	process_map_mtx.unlock();

	// Save child pid to parent ax
	regs.rax.x = static_cast<kiv_os::THandle>(last_handle);
}

void ProcessManager::createThread(kiv_hal::TRegisters &regs)
{
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

	Process* currentProcess = getRunningProcess();
	size_t parent_pid = currentProcess->pid;
	kiv_os::THandle parent_handle = currentProcess->handle;

	Process* newProcess = new Process(processes[parent_pid]->userfunc_name, parent_pid, true);
	kiv_hal::TRegisters child_context{ 0 };
	child_context.rdi.r = regs.rdi.r; // pointer to shared data
	child_context.rax.x = stdin_handle; // stdin
	child_context.rbx.x = stdout_handle; // stdout

	process_map_mtx.lock();
	newProcess->start(child_context, programAddress);
	processes[newProcess->pid] = newProcess;
	handles[++last_handle] = newProcess->pid;
	newProcess->handle = last_handle;
	newProcess->parent_handle = parent_handle;
	process_map_mtx.unlock();

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
	std::vector<Process*> waiting_for(handles_size);

	for (int i = 0; i < handles_size; i++)
	{
		if (handles.count(registered_handles[i]) == 0)
		{
			regs.flags.carry = 1;
			regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
			return;
		}
		waiting_for[i] = processes[handles[registered_handles[i]]];
		if (waiting_for[i]->state == ProcessState::stopped)
		{
			regs.rax.r = static_cast<uint64_t>(registered_handles[i]);
			return;
		}
	}

	while (true)
	{
		std::unique_lock<std::mutex> lk(Process::endMtx);
		Process::endCond.wait_for(lk, std::chrono::milliseconds(10));
		for (int i = 0; i < handles_size; i++)
		{
			if (waiting_for[i]->state == ProcessState::stopped)
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
	uint16_t exitCode = static_cast<uint16_t>(regs.rcx.x);
	process_map_mtx.lock();
	Process* thisProcess = getRunningProcess();
	process_map_mtx.unlock();
	thisProcess->stop(exitCode);
}

void ProcessManager::handleReadExitCode(kiv_hal::TRegisters &regs)
{
	// dx = process / thread handle
	// OUT: cx = exitcode
	kiv_os::THandle process_handle = static_cast<uint16_t>(regs.rdx.x);
	regs.flags.carry = 0;

	if (handles.count(process_handle) == 0)
	{
		regs.flags.carry = 1;
		regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		return;
	}
	Process* child_process = processes[handles[process_handle]];
	if (child_process->state != ProcessState::stopped)
	{
		regs.flags.carry = 1;
		regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Invalid_Argument);
		return;
	}
	uint16_t exit_code = child_process->exitCode;
	regs.rcx.x = exit_code;
	removeProcess(process_handle);
}

void ProcessManager::removeProcess(kiv_os::THandle handle)
{
	process_map_mtx.lock();
	size_t pid = handles[handle];
	Process* process_to_remove = processes[pid];
	handles.erase(handle);
	processes.erase(pid);
	delete process_to_remove;
	process_map_mtx.unlock();
}

std::string ProcessManager::getProcessTable()
{
	process_map_mtx.lock();
	std::ostringstream result;
	result << "PID\tPPID\tSTATUS\tTYPE\tCOMMAND" << std::endl;
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
		std::string type = process->is_thread ? "THREAD" : "PROCESS";
		result << process->handle << "\t" << process->parent_handle << "\t" << process_state << "\t" << type << "\t" << process->userfunc_name << std::endl;
	}
	process_map_mtx.unlock();
	return result.str();
}