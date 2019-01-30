#pragma once

#include "..\api\api.h"
#include "iohandle.h"

#include <memory>

// @author: Lukas Cerny

struct STDHandle {
	kiv_os::THandle in;
	kiv_os::THandle out;
};

STDHandle Register_STD();

void Handle_IO(kiv_hal::TRegisters &regs);