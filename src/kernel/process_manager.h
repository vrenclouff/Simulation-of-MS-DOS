#pragma once
#include <map>
#include <thread>
#include <mutex>
#include "process.h"
#include <memory>

class Process_Manager
{
private:
	std::mutex mtx; // mutex for creating new process
	std::map<size_t, std::unique_ptr<Process>> processes;
	std::map<kiv_os::THandle, size_t> handles; // tid mapped to THandle
	kiv_os::THandle last_handle = 0;
	size_t _get_running_tid();
	Process* _get_running_process();
	Thread* _get_running_thread();
	Thread* get_thread_by_tid(size_t tid);
	Process* get_process_by_tid(size_t tid);
	void remove_handle(kiv_os::THandle handle);
	void remove_process(kiv_os::THandle handle);

public:
	Process* get_running_process();
	Thread* get_running_thread();
	void sys_call(kiv_hal::TRegisters &regs);
	// Syscall handlers
	void handle_clone(kiv_hal::TRegisters &regs);
	void create_process(kiv_hal::TRegisters &regs, bool first_process = false);
	void create_thread(kiv_hal::TRegisters &regs);
	void handle_wait_for(kiv_hal::TRegisters &regs);
	void handle_exit(kiv_hal::TRegisters &regs);
	void handle_read_exit_code(kiv_hal::TRegisters &regs);
	void register_signal_handler(kiv_hal::TRegisters &regs);
	void shutdown(kiv_hal::TRegisters &regs);
	std::string get_process_table();
};