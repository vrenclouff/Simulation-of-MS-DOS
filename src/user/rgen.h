#pragma once

#include "..\api\api.h"

void wait_For_Ctrlz(const kiv_hal::TRegisters &regs);

extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs);
