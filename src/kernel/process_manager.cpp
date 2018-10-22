#include "process_manager.h"
#include <iostream>
#include <vector>

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
	size_t parent_pid = 0;
	kiv_os::THandle stdin_handle = regs.rbx.e >> 16;
	kiv_os::THandle stdout_handle = regs.rbx.e & 0x00ff;

	if (!first_process)
	{
		Process* currentProcess = getRunningProcess();
		parent_pid = currentProcess->pid;
	}
	
	char* funcName = reinterpret_cast<char*>(regs.rdx.r);

	Process* newProcess = new Process(funcName, parent_pid);
	kiv_hal::TRegisters child_context{ 0 };
	child_context.rdi.r = regs.rdi.r; // function args
	child_context.rax.x = stdin_handle; // stdin
	child_context.rbx.x = stdout_handle; // stdout

	create_process_mtx.lock();
	newProcess->start(child_context);
	processes[newProcess->pid] = newProcess;
	handles[++last_handle] = newProcess->pid;
	create_process_mtx.unlock();

	// Save child pid to parent ax
	regs.rax.x = static_cast<kiv_os::THandle>(last_handle);
}

void ProcessManager::createThread(kiv_hal::TRegisters &regs)
{

}

void ProcessManager::handleWaitFor(kiv_hal::TRegisters &regs)
{
	// RDX = array of handles
	// RCX = array size
	kiv_os::THandle *registered_handles = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r);
	size_t handles_size = static_cast<size_t>(regs.rcx.r);
	std::vector<Process*> waiting_for(handles_size);

	for (int i = 0; i < handles_size; i++)
	{
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
	create_process_mtx.lock();
	Process* thisProcess = getRunningProcess();
	create_process_mtx.unlock();
	thisProcess->stop(exitCode);
}

void ProcessManager::handleReadExitCode(kiv_hal::TRegisters &regs)
{
	// dx = process / thread handle
	// OUT: cx = exitcode
	kiv_os::THandle process_handle = static_cast<uint16_t>(regs.rdx.x);
	regs.flags.carry = 0;
	regs.rax.x = static_cast<uint16_t>(kiv_os::NOS_Error::Success);

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
	size_t pid = handles[handle];
	Process* process_to_remove = processes[pid];
	handles.erase(handle);
	processes.erase(pid);
	delete process_to_remove;
}
