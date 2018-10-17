#pragma once

#include "..\api\hal.h"
#include "..\api\api.h"

#include <cstdint>
#include <functional>
#include <string_view>
#include <ctime>
#include <vector>

namespace kiv_fs {

	enum class FAT_Version : std::uint8_t {
		FAT16
	};

	enum FAT_Attributes : uint16_t {
		Free = 0x0000,
		Bad = 0xFFF7,
		End = 0xFFFF,
	};

	struct FATEntire_Directory {
		char file_name[8];
		char extension[3];
		uint8_t attributes;
		char reserved[10];
		uint16_t time;
		uint16_t date;
		uint16_t first_cluster;
		uint32_t size;
	};

	struct FATBoot_Block {
		std::string description;
		uint16_t bytes_per_sector;
		uint8_t number_per_allocation_unit;
		uint8_t number_of_fat;
		uint8_t reserved_sectors;
		uint16_t number_of_root_directory_entries;
		uint64_t absolute_number_of_sectors;
		uint16_t blocks_for_fat;
		uint32_t sectors_per_track;
		uint32_t heads;
		std::string volume_label;
		std::string file_system_identifier;
	};


	void format_disk(const FAT_Version version, void* boot_block, const kiv_hal::TDrive_Parameters &params);
	bool is_formatted(const void* sector);

	void boot_block(FATBoot_Block& boot_block, const uint16_t bytes_per_sector, const void* sector);
	void entire_directory(std::vector<FATEntire_Directory>& entire_directories, const FATBoot_Block& boot_block, void* sector);

	uint16_t root_directory_addr(const FATBoot_Block& boot_block);
	uint8_t root_directory_size(const FATBoot_Block& boot_block);
}