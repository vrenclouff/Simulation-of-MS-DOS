#pragma once
#include <map>
#include <thread>
#include <mutex>
#include "process.h"

class ProcessManager
{
private:
	std::mutex process_map_mtx; // mutex for creating new process
	std::map<size_t, Process*> processes;
	std::map<kiv_os::THandle, size_t> handles; // processes mapped to THandle
	kiv_os::THandle last_handle = 0;
	void removeProcess(kiv_os::THandle handle);
public:
	Process* getRunningProcess();
	void SysCall(kiv_hal::TRegisters &regs);
	// Syscall handlers
	void handleClone(kiv_hal::TRegisters &regs);
	void createProcess(kiv_hal::TRegisters &regs, bool first_process = false);
	void createThread(kiv_hal::TRegisters &regs);
	void handleWaitFor(kiv_hal::TRegisters &regs);
	void handleExit(kiv_hal::TRegisters &regs);
	void handleReadExitCode(kiv_hal::TRegisters &regs);
	std::string getProcessTable();
};