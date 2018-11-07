#pragma once

#include "fat.h"

#include <functional>
#include <map>


class IOManager {

	struct Drive_Desc {
		uint8_t id;
		kiv_fs::FATBoot_Block boot_block;
	};

private:
	std::map<std::string, Drive_Desc> registred_drivers;

public:

	bool register_drive(const std::string volume, const uint8_t id, const kiv_fs::FATBoot_Block& book_block);
	bool open(const std::string drive_volume, const std::vector<std::string> path_components);
};