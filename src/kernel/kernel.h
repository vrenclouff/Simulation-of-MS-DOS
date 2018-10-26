#pragma once


#include "..\api\hal.h"
#include "..\api\api.h"

void Set_Error(const bool failed, kiv_hal::TRegisters &regs);
void __stdcall Bootstrap_Loader(kiv_hal::TRegisters &context);
void Call_User_Function(char* funcname, kiv_hal::TRegisters regs);
