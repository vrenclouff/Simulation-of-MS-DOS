#pragma once

#include "..\api\hal.h"
#include "..\api\api.h"

#include <cstdint>
#include <functional>
#include <ctime>
#include <vector>

// @author: Lukas Cerny

#define MULTIPLY_CONST	0x02

#define START_OF_FAT std::div_t{ 1,  MULTIPLY_CONST * MULTIPLY_CONST }


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
		// std::string description;
		uint16_t bytes_per_sector;
		// uint8_t number_per_allocation_unit;
		uint8_t number_of_fat;
		uint16_t blocks_for_fat;
		// uint8_t reserved_sectors;
		uint16_t number_of_root_directory_entries;
		// uint64_t absolute_number_of_sectors;
		// uint32_t sectors_per_track;
		// uint32_t heads;
		// std::string volume_label;
		// std::string file_system_identifier;
	};

	struct Drive_Desc {
		uint8_t id;
		kiv_fs::FATBoot_Block boot_block;
	};

	struct File_Desc {
		kiv_fs::FATEntire_Directory entire_dir = { 0 };
		std::vector<uint16_t> sectors;
	};

	enum class Edit_Type : uint8_t {
		Add = 1,
		Del = 2,
		Edit = 3,
	};

	bool new_entire_dir(kiv_fs::FATEntire_Directory& entry, const std::string name, const uint8_t attributes, kiv_hal::NDisk_Status& error);

	void format_disk(const FAT_Version version, void* boot_block, const kiv_hal::TDrive_Parameters &params);
	bool is_formatted(const void* sector);

	void boot_block(FATBoot_Block& boot_block, const size_t bytes_per_sector, const void* sector);
	void entire_directory(std::vector<FATEntire_Directory>& entire_directories, const size_t bytes_per_sector, void* sector);

	bool find_entire_dirs(const kiv_fs::Drive_Desc drive, std::vector<kiv_fs::File_Desc>& files, std::vector<std::string> components);

	uint16_t offset(const FATBoot_Block& boot_block);

	std::vector<uint16_t> load_sectors(const kiv_fs::Drive_Desc& drive, const kiv_fs::FATEntire_Directory& entry_dir);
	void sector_to_fat_offset(std::vector<std::div_t>& fat_offsets, const std::vector<uint16_t> sectors, const size_t bytes_per_sector, const uint16_t offset);

	bool is_entry_root(const kiv_fs::FATBoot_Block& boot, const kiv_fs::FATEntire_Directory& entry);
	uint16_t root_directory_addr(const FATBoot_Block& boot_block);
	uint8_t root_directory_size(const FATBoot_Block& boot_block);

	bool create_file(const kiv_fs::Drive_Desc& drive, const kiv_fs::File_Desc& parrent, kiv_fs::File_Desc& file);

	bool find_free_sectors(std::vector<std::div_t>& fat_offsets, const uint8_t drive_id, const std::div_t sector, const size_t count, const size_t bytes_per_sector, kiv_hal::NDisk_Status& disk_status);
	bool save_to_dir(const uint8_t drive_id, const std::vector<uint16_t> sectors, const size_t bytes_per_sector, const kiv_fs::FATEntire_Directory entire_dir, const kiv_fs::Edit_Type type, kiv_hal::NDisk_Status& disk_status);
	bool save_to_fat(const uint8_t drive_id, const std::vector<std::div_t> fat_offsets, const size_t bytes_per_sector, const uint16_t offset, std::vector<uint16_t>& sectors, uint16_t& first_sector, kiv_hal::NDisk_Status& disk_status);

	void remove_sectors_in_fat(const kiv_fs::FATEntire_Directory& entire_dir, const size_t bytes_per_sector, const uint8_t drive_id, kiv_hal::NDisk_Status& disk_status);
}