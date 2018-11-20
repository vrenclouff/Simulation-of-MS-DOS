#include "process.h"
#include <Windows.h>
#include "common.h"


void defaultTerminateHandle(const kiv_hal::TRegisters &context)
{
}

Process::Process(std::string userfunc_name, size_t parent_pid) :
	userfunc_name(userfunc_name), parent_pid(parent_pid)
{
	state = ProcessState::prepared;
}

size_t Process::startThread(kiv_hal::TRegisters child_context, kiv_os::TThread_Proc address)
{
	Thread *thread = new Thread(address, child_context);
	thread->handlers[kiv_os::NSignal_Id::Terminate] = reinterpret_cast<kiv_os::TThread_Proc>(defaultTerminateHandle);
	thread->start();
	threads[thread->tid] = thread;
	if (state == ProcessState::prepared)
	{
		state = ProcessState::running;
		pid = thread->tid;
	}
	return thread->tid;
}

void Process::stopThread(uint16_t code, size_t tid)
{
	threads[tid]->stop(code);
}

void Process::cleanThread(size_t tid)
{
	Thread* thread_to_clean = threads[tid];
	threads.erase(tid);
	WaitForSingleObject(thread_to_clean->thread_obj->native_handle(), INFINITE);
	delete thread_to_clean;
	if (threads.size() == 0)
	{
		state = ProcessState::stopped;
	}
}