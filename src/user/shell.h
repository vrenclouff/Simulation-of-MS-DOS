#pragma once

#include "..\api\api.h"
#include <atomic>

// @author: template

extern std::atomic_bool shell_echo;

extern "C" size_t __stdcall shell(const kiv_hal::TRegisters &regs);
