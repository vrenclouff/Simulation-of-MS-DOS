#include "thread.h"
#include <Windows.h>

// @author: Petr Volf

std::condition_variable Thread::endCond;

Thread::Thread(kiv_os::TThread_Proc func_addr, kiv_hal::TRegisters thread_context) :
	func_addr(func_addr), context(thread_context), state(ThreadState::prepared), tid(0), exitCode(0), thread_obj({}) {}

void Thread::start() {
	thread_obj = std::thread(func_addr, context);

	tid = std::hash<std::thread::id>()(thread_obj.get_id());
	state = ThreadState::running;
}

void Thread::stop(uint16_t code) {
	exitCode = code;
	state = ThreadState::stopped;
	Thread::endCond.notify_all();
	TerminateThread(thread_obj.native_handle(), 0);
}