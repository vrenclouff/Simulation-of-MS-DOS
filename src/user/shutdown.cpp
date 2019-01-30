#include "shutdown.h"
#include "rtl.h"

// @author: Petr Volf

size_t __stdcall shutdown(const kiv_hal::TRegisters &regs) {
	kiv_os_rtl::Shutdown();
	kiv_os_rtl::Exit(0);
	return 0;
}
