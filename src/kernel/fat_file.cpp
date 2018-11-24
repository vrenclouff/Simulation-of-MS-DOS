
#include "fat_file.h"
#include "fat_tools.h"
#include "fat.h"

#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm> 
#include <map>

//*********** PRIVATE ***********


uint16_t FATFile::offset() const {
	const auto root_dir_addr = kiv_fs::root_directory_addr(_boot_block);
	const auto number_of_blocks = kiv_fs::root_directory_size(_boot_block);
	return root_dir_addr + number_of_blocks - 0x2;
}


//*********** PUBLIC ************

uint32_t FATFile::size() {
	return _entire_dir.size;
}

std::tm FATFile::date() {
	return fat_tool::makedate(_entire_dir.date, _entire_dir.time);
}

void FATFile::write() {

	auto div = std::div(_entire_dir.size, _boot_block.bytes_per_sector);
	uint16_t size_in_blocks = (div.rem ? div.quot + 1 : div.quot);
	const uint16_t bytes_per_sector = _boot_block.bytes_per_sector;

	std::vector<std::div_t> allocated_space;

	std::div_t fat_offset = { 1,  MULTIPLY_CONST * MULTIPLY_CONST };

	std::vector<unsigned char> fat(bytes_per_sector);
	_hard_disk(fat.data(), fat_offset.quot, kiv_hal::NDisk_IO::Read_Sectors);

	do {
		if (fat_offset.rem > bytes_per_sector) {
			fat.resize(fat.size() + bytes_per_sector);
			_hard_disk(fat.data() + bytes_per_sector, ++fat_offset.quot, kiv_hal::NDisk_IO::Read_Sectors);
			fat_offset.rem = 0;
		}
		uint16_t fat_cell = fat.at(fat_offset.rem) | fat.at(fat_offset.rem + 1) << 8;
		if (fat_cell == kiv_fs::FAT_Attributes::Free) {
			allocated_space.push_back(fat_offset);
		}
		fat_offset.rem += sizeof(decltype(fat_cell));
	} while (allocated_space.size() < size_in_blocks);

	if (allocated_space.empty()) {
		// TODO ERROR no space for file
		return;
	}

	auto find_sector = [&](const std::div_t& ofst) {
		return ((((ofst.quot - 1) * _boot_block.bytes_per_sector) + ofst.rem) / MULTIPLY_CONST);
	};

	std::vector<uint16_t> real_clusters;
	const auto _offset = offset();

	auto actual_fat = allocated_space.at(0);
	auto fake_sector = find_sector(actual_fat);
	real_clusters.push_back(fake_sector + _offset);

	_entire_dir.first_cluster = fake_sector;

	for (auto next_fat = allocated_space.begin() + 1; next_fat != allocated_space.end(); next_fat++) {
		fake_sector = find_sector(*next_fat);
		real_clusters.push_back(fake_sector + _offset);
		auto index = actual_fat.rem;
		fat[index] = fake_sector & 0xff;
		fat[index + 1] = (fake_sector & 0xff00) >> 8;
		actual_fat = *next_fat;
	}

	fat[actual_fat.rem] = fat[actual_fat.rem + 1] = kiv_fs::FAT_Attributes::End >> 8;

	_hard_disk(fat.data(), actual_fat.quot, kiv_hal::NDisk_IO::Write_Sectors);

	uint16_t cluster_offset = 0;
	for (auto& sector : real_clusters) {
		if (_entire_dir.size - cluster_offset < bytes_per_sector) {
			_data.resize(size_in_blocks * bytes_per_sector);
		}
		_hard_disk(_data.data() + cluster_offset, sector, kiv_hal::NDisk_IO::Write_Sectors);
		cluster_offset += bytes_per_sector;
	}

	const auto root_dir_addr = kiv_fs::root_directory_addr(_boot_block);
	const auto root_dir_size = kiv_fs::root_directory_size(_boot_block);
	const auto max_dir_entries_per_block = static_cast<uint8_t>(bytes_per_sector / sizeof(kiv_fs::FATEntire_Directory));
	std::vector<unsigned char> root_dir(bytes_per_sector);
	for (auto sector = root_dir_addr; sector < root_dir_size + root_dir_addr; sector++) {
		_hard_disk(root_dir.data(), sector, kiv_hal::NDisk_IO::Read_Sectors);
		std::vector<kiv_fs::FATEntire_Directory> dir_entries;
		kiv_fs::entire_directory(dir_entries, _boot_block.bytes_per_sector, root_dir.data());
		if (dir_entries.size() < max_dir_entries_per_block) {
			dir_entries.push_back(_entire_dir);
			dir_entries.resize(max_dir_entries_per_block);
			_hard_disk(reinterpret_cast<unsigned char*>(dir_entries.data()), sector, kiv_hal::NDisk_IO::Write_Sectors);
			break;
		}
	}
}

std::vector<unsigned char>& FATFile::read() {

	const auto bytes_per_sector = _boot_block.bytes_per_sector;
	const auto _offset = offset();
	const auto sectors = std::vector<uint16_t>(); // load_sectors(_entire_dir, bytes_per_sector, _offset);

	const auto div = std::div(_entire_dir.size, bytes_per_sector);
	const auto full_size = (div.rem ? div.quot + 1 : div.quot) * bytes_per_sector;
	_data.resize(full_size + 1);

	uint16_t offset = 0;
	for (auto& sector : sectors) {
		_hard_disk(_data.data() + offset, sector, kiv_hal::NDisk_IO::Read_Sectors);
		offset += bytes_per_sector;
	}
	_data.at(_entire_dir.size) = 0x00;
	_data.resize(_entire_dir.size + 1);
	_data.shrink_to_fit();

	return _data;
}


std::string FATFile::to_string() {

	std::stringstream wss;
	const auto is_dir = fat_tool::is_attr(_entire_dir.attributes, kiv_os::NFile_Attributes::Directory);
	auto file_name = fat_tool::rtrim(std::string(_entire_dir.file_name, 8));
	auto extension = fat_tool::rtrim(std::string(_entire_dir.extension, 3));

	wss
		<< std::put_time(&date(), "%d. %m. %Y %H:%M:%S") << " "
		<< (is_dir ? std::to_string(0) : std::to_string(_entire_dir.size)) << " "
		//<< (is_dir ? "<DIR>" : "     " + std::to_string(_entire_dir.size)) << " "
		<< file_name << "." << extension 
		<< std::endl;

	return wss.str();
}
