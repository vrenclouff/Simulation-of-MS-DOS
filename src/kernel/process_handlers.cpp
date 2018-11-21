#include "process_handlers.h"

void defaultTerminateHandle(const kiv_hal::TRegisters &context)
{
	Thread* this_thread = process_manager->getRunningThread();
	TerminateThread(this_thread->thread_obj->native_handle(), 0);
}