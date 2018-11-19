#pragma once
#include <string>
#include "../api/hal.h"
#include "../api/api.h"
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include "thread.h"
enum class ProcessState
{
	prepared = 1,
	running,
	stopped
};

class Process
{
public:
	size_t pid;
	size_t parent_pid;
	kiv_os::THandle handle;
	kiv_os::THandle parent_handle;
	ProcessState state;
	std::string userfunc_name;
	std::string working_dir;
	std::map<size_t, Thread*> threads;

	Process(std::string userfunc_name, size_t parent_pid);
	size_t startThread(kiv_hal::TRegisters thread_context, kiv_os::TThread_Proc address);
	void stopThread(uint16_t exitCode, size_t tid);
	void cleanThread(size_t tid);
};