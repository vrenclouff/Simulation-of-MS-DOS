#pragma once

#include "fat.h"


#define MULTIPLY_CONST	0x02

class FATFile {

private:
	kiv_fs::FATEntire_Directory& _entire_dir;
	kiv_fs::FATBoot_Block& _boot_block;
	std::function<void(unsigned char* data, uint16_t sector, kiv_hal::NDisk_IO operation)> _hard_disk;
	std::vector<unsigned char> _data;

	uint16_t offset() const;


public:
	FATFile(
		kiv_fs::FATEntire_Directory &entire_dir, 
		kiv_fs::FATBoot_Block &boot_block,
		std::function<void(unsigned char* data, uint16_t sector, kiv_hal::NDisk_IO operation)> hard_disk
	) : _entire_dir(entire_dir), _boot_block(boot_block), _hard_disk(hard_disk), _data(std::vector<unsigned char>()) {}

	FATFile(
		kiv_fs::FATEntire_Directory &entire_dir,
		kiv_fs::FATBoot_Block &boot_block,
		std::vector<unsigned char> data,
		std::function<void(unsigned char* data, uint16_t sector, kiv_hal::NDisk_IO operation)> hard_disk
	) : _entire_dir(entire_dir), _boot_block(boot_block), _data(data), _hard_disk(hard_disk) {}
	
	FATFile() : _entire_dir(kiv_fs::FATEntire_Directory()), _boot_block(kiv_fs::FATBoot_Block()) {}
	
	void write();
	std::vector<unsigned char>& read();

	uint32_t size();
	std::tm date();
	std::string to_string();
};