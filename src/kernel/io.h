#pragma once

#include "..\api\api.h"
#include "io_manager.h"
#include "fat_file.h"

//#include <memory>


//inline std::unique_ptr<IOManager> IO_Manager;

bool Open_File(FATFile & fat_file, std::string file_name, const kiv_os::NOpen_File open_flags, const kiv_os::NFile_Attributes attributes);
size_t Read_File(kiv_os::THandle file_handle, char* buffer, size_t buffer_size);

void Handle_IO(kiv_hal::TRegisters &regs);
