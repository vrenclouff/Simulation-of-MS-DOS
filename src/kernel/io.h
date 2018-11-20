#pragma once

#include "..\api\api.h"
#include "iohandle.h"

#include <memory>

struct STDHandle {
	kiv_os::THandle in;
	kiv_os::THandle out;
};

STDHandle Register_STD();

IOHandle* Open_File(std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes);

void Handle_IO(kiv_hal::TRegisters &regs);

namespace io {
	std::string main_drive();
	bool register_drive(const std::string volume, const uint8_t id, const kiv_fs::FATBoot_Block& book_block);
}