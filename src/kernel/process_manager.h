#pragma once
#include <map>
#include <thread>
#include <mutex>
#include "process.h"

class ProcessManager
{
private:
	std::recursive_mutex mtx; // mutex for creating new process
	std::map<size_t, Process*> processes;
	std::map<kiv_os::THandle, size_t> handles; // tid mapped to THandle
	Thread* overriden_running_thread = nullptr;
	kiv_os::THandle last_handle = 0;
	size_t _getRunningTid();
	Process* _getRunningProcess();
	Thread* _getRunningThread();
	Thread* getThreadByTid(size_t tid);
	Process* getProcessByTid(size_t tid);
	void removeHandle(kiv_os::THandle handle);
	void removeProcess(kiv_os::THandle handle);

public:
	Process* getRunningProcess();
	Thread* getRunningThread();
	void SysCall(kiv_hal::TRegisters &regs);
	// Syscall handlers
	void handleClone(kiv_hal::TRegisters &regs);
	void createProcess(kiv_hal::TRegisters &regs, bool first_process = false);
	void createThread(kiv_hal::TRegisters &regs);
	void handleWaitFor(kiv_hal::TRegisters &regs);
	void handleExit(kiv_hal::TRegisters &regs);
	void handleReadExitCode(kiv_hal::TRegisters &regs);
	void registerSignalHandler(kiv_hal::TRegisters &regs);
	void shutdown(kiv_hal::TRegisters &regs);
	std::string getProcessTable();
};