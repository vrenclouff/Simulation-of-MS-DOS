#pragma once

#include "..\api\api.h"

/*
	sestavi frekvencni tabulku vsech zadanych bytu
*/
extern "C" size_t __stdcall freq(const kiv_hal::TRegisters &regs);
