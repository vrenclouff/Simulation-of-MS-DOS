#pragma once

#include "..\api\api.h"

/*
	vypise obsah zadaneho adresare
*/
extern "C" size_t __stdcall dir(const kiv_hal::TRegisters &regs);
