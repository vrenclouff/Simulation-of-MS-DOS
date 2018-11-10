#pragma once

#include "..\api\api.h"
#include "io_manager.h"
#include "fat_file.h"

#include <memory>

kiv_os::THandle Register_STDIN();
kiv_os::THandle Register_STDOUT();

using THandle_Proc = size_t(*)(char* buffer, size_t buffer_size);

bool Open_File(FATFile & fat_file, std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes);

void Handle_IO(kiv_hal::TRegisters &regs);
