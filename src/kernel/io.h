#pragma once

#include "..\api\api.h"
#include "iohandle.h"

#include <memory>

struct STDHandle {
	kiv_os::THandle in;
	kiv_os::THandle out;
};

STDHandle Register_STD();

void Handle_IO(kiv_hal::TRegisters &regs);

namespace io {
	std::string main_drive();
	bool register_drive(const std::string volume, const uint8_t id, const kiv_fs::FATBoot_Block& book_block);
}