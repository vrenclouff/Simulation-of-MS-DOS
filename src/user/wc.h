#pragma once

#include "..\api\api.h"

/*
	find /v /c "" - wc
	vraci pocet radek vstupu
*/
extern "C" size_t __stdcall wc(const kiv_hal::TRegisters &regs);
