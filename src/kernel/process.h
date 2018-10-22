#pragma once
#include <string>
#include "../api/hal.h"
#include "../api/api.h"
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
// TODO enum of process states
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
	ProcessState state;
	uint16_t exitCode;
	std::string userfunc_name;
	kiv_hal::TRegisters context;
	std::thread* thread_obj;
	std::map<size_t, Process*> childs;
	// std::string name;
	// TODO workdir
	// TODO syscall struct params, parent id
	Process(std::string userfunc_name, size_t parent_pid);
	void start(kiv_hal::TRegisters child_context);
	void stop(uint16_t exitCode);
	static std::condition_variable endCond; // condition for process end
	static std::mutex endMtx; // endCond mutex
};