#pragma once

#include "fat.h"

#include <map>
#include <string>

namespace drive {
	std::string main_volume();
	void save(const std::string volume, const uint8_t id, const kiv_fs::FATBoot_Block& book_block);
	kiv_fs::Drive_Desc at(const std::string volume);
}