#pragma once

#include "..\api\api.h"

extern bool shell_echo;

extern "C" size_t __stdcall shell(const kiv_hal::TRegisters &regs);

//cd nemuze byt externi program, ale vestavny prikaz shellu!
