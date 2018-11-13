#pragma once

#include "fat.h"

#include <functional>
#include <map>

namespace io {

	bool register_drive(const std::string volume, const uint8_t id, const kiv_fs::FATBoot_Block& book_block);
	bool open(kiv_fs::Drive_Desc& drive, kiv_fs::FATEntire_Directory& entire_dir, const std::string drive_volume, const std::vector<std::string> path_components);
}