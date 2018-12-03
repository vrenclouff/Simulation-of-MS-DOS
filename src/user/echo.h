#pragma once

#include "..\api\api.h"

/*
	vypise zadany vstup, pripadne vypne/zapne zobrazovani aktualniho pracovniho adresare
*/
extern "C" size_t __stdcall echo(const kiv_hal::TRegisters &regs);
