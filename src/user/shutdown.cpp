#include "shutdown.h"
#include "rtl.h"

size_t __stdcall shutdown(const kiv_hal::TRegisters &regs) {
	return kiv_os_rtl::Shutdown();
}
