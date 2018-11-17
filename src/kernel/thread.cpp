#include "thread.h"

std::condition_variable Thread::endCond;
std::mutex Thread::endMtx;

Thread::Thread(kiv_os::TThread_Proc func_addr, kiv_hal::TRegisters thread_context): 
	func_addr(func_addr), context(thread_context)
{
	state = ThreadState::prepared;
}

void Thread::start()
{
	thread_obj = new std::thread(func_addr, context);

	tid = std::hash<std::thread::id>()(thread_obj->get_id());
	state = ThreadState::running;
}

void Thread::stop(uint16_t code)
{
	exitCode = code;
	state = ThreadState::stopped;
	std::unique_lock<std::mutex> lk(Thread::endMtx);
	Thread::endCond.notify_all();
	lk.unlock();
	// TODO how to kill current thread?
}