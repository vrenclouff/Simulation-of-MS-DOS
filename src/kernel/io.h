#pragma once

#include "..\api\api.h"
#include "io_manager.h"
#include "fat_file.h"

#include <memory>

struct STDHandle {
	kiv_os::THandle in;
	kiv_os::THandle out;
};

STDHandle Register_STD();

using THandle_Proc = size_t(*)(char* buffer, size_t buffer_size);

kiv_os::THandle Open_File(std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes);

void Handle_IO(kiv_hal::TRegisters &regs);
