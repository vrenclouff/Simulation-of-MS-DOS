#include "process.h"

std::condition_variable Process::endCond;
std::mutex Process::endMtx;

Process::Process(std::string userfunc_name, size_t parent_pid, bool is_thread) :
	userfunc_name(userfunc_name), parent_pid(parent_pid), is_thread(is_thread)
{
	state = ProcessState::prepared;
}

void Process::start(kiv_hal::TRegisters child_context, kiv_os::TThread_Proc address)
{
	context = child_context;

	thread_obj = new std::thread(address, context);

	pid = std::hash<std::thread::id>()(thread_obj->get_id());
	state = ProcessState::running;
}

void Process::stop(uint16_t code)
{
	exitCode = code;
	state = ProcessState::stopped;
	Process::endCond.notify_all();
	// TODO how to kill current thread?
}