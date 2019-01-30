#pragma once

#include "iohandle.h"

// @author: Lukas Cerny

namespace io {
	bool is_exist_dir(std::string path);
	IOHandle* Open_File(std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes, kiv_os::NOS_Error& error);
	bool Remove_File(std::string absolute_path, kiv_os::NOS_Error& error);
}