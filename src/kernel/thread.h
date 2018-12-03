#pragma once
#include <string>
#include "../api/hal.h"
#include "../api/api.h"
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <iostream>

enum class ThreadState
{
	prepared = 1,
	running,
	stopped
};

class Thread
{
public:
	size_t tid;
	ThreadState state;
	uint16_t exitCode;
	std::thread thread_obj;
	kiv_os::TThread_Proc func_addr;
	std::map<kiv_os::NSignal_Id, kiv_os::TThread_Proc> handlers;
	kiv_hal::TRegisters context;

	Thread(kiv_os::TThread_Proc func_addr, kiv_hal::TRegisters thread_context);
	~Thread()
	{
		handlers.clear();
		if (thread_obj.joinable())
		{
			thread_obj.detach();
		}
	};
	void start();
	void stop(uint16_t exitCode);
	static std::condition_variable endCond; // condition for process end
};