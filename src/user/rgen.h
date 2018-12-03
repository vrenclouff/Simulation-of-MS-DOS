#pragma once

#include "..\api\api.h"

/*
	cekani na ctrl+z
*/
void wait_For_Ctrlz(const kiv_hal::TRegisters &regs);

/*
	generuje nahodna cisla v plovouci radce tak dlouho, dokud neprijde ctr¾+z
*/
extern "C" size_t __stdcall rgen(const kiv_hal::TRegisters &regs);
