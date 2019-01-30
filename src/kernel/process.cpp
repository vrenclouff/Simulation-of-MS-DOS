#include "process.h"
#include <Windows.h>
#include "common.h"

// @author: Petr Volf

void defaultTerminateHandle(const kiv_hal::TRegisters &context)
{
}

Process::Process(std::string userfunc_name, size_t parent_pid) :
	userfunc_name(userfunc_name), parent_pid(parent_pid), state(ProcessState::prepared), handle(0), parent_handle(0), pid(0) {}

size_t Process::start_thread(kiv_hal::TRegisters child_context, kiv_os::TThread_Proc address)
{
	std::unique_ptr<Thread> thread = std::make_unique<Thread>(address, child_context);
	thread->handlers[kiv_os::NSignal_Id::Terminate] = reinterpret_cast<kiv_os::TThread_Proc>(defaultTerminateHandle);
	thread->start();
	size_t temp_tid = thread->tid;
	threads.insert(std::pair<size_t, std::unique_ptr<Thread>>(temp_tid, std::move(thread)));
	if (state == ProcessState::prepared)
	{
		state = ProcessState::running;
		pid = temp_tid;
	}
	return temp_tid;
}

void Process::stop_thread(uint16_t code, size_t tid)
{
	threads[tid]->stop(code);
}

void Process::clean_thread(size_t tid)
{
	Thread* thread_to_clean = threads[tid].get();
	thread_to_clean->thread_obj.join();
	//WaitForSingleObject(thread_to_clean->thread_obj->native_handle(), INFINITE);
	threads.erase(tid);
	if (threads.size() == 0)
	{
		state = ProcessState::stopped;
	}
}