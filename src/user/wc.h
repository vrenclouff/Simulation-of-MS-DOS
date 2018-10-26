#pragma once

#include "..\api\api.h"

// find /v /c "" - wc - word count
extern "C" size_t __stdcall wc(const kiv_hal::TRegisters &regs);
