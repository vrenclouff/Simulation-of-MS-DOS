#include "fat.h"

#include <numeric>

void format_fat16(unsigned char* boot_block, const kiv_hal::TDrive_Parameters& params) {

	// Clean boot block
	for (uint16_t i = 0; i < params.bytes_per_sector; i++) {
		boot_block[i] = 0;
	}

	// Part of the bootstrap program. (3 bytes)
	boot_block[0x00] = '.';
	boot_block[0x01] = '<';
	boot_block[0x02] = '.';

	// Optional manufacturer description. (8 bytes)
	boot_block[0x03] = 'K';
	boot_block[0x04] = 'I';
	boot_block[0x05] = 'V';
	boot_block[0x06] = '_';
	boot_block[0x07] = 'F';
	boot_block[0x08] = 'A';
	boot_block[0x09] = 'T';
	boot_block[0x0a] = 32;

	// Number of bytes per block (almost always 512). (2 bytes)
	// tam  (x & 0x00ff) << 8 | (x & 0xff00) >> 8;
	// zpet (x & 0xff00) >> 8 | (x & 0x00ff) << 8;
	boot_block[0x0b] = params.bytes_per_sector & 0xFF;
	boot_block[0x0c] = params.bytes_per_sector >> 8;


	// Number of blocks per allocation unit. (1 byte)
	boot_block[0x0d] = 0x01;

	// Number of reserved blocks. 
	boot_block[0x0e] = 0x01;

	// Number of File Allocation Tables. (1 byte)
	boot_block[0x10] = 1;

	// Number of root directory entries (including unused ones). (2 bytes)
	boot_block[0x11] = 0x40;
	boot_block[0x12] = 0x00;

	// Total number of blocks in the entire disk. (2 bytes)
	boot_block[0x13] = (uint16_t)params.absolute_number_of_sectors & 0xFF;
	boot_block[0x14] = (uint16_t)params.absolute_number_of_sectors >> 8;

	// The number of blocks occupied by one copy of the File Allocation Table. (2 bytes)
	boot_block[0x16] = 0x14;
	boot_block[0x17] = 0x00;

	// The number of blocks per track. (2 bytes)
	boot_block[0x18] = params.sectors_per_track & 0xFF;
	boot_block[0x19] = params.sectors_per_track >> 8;

	// The number of heads (disk surfaces). (2 bytes)
	boot_block[0x1a] = params.heads & 0xFF;
	boot_block[0x1b] = params.heads >> 8;

	// Volume Label. (11 bytes)
	boot_block[0x2b] = 'v';
	boot_block[0x2c] = 'o';
	boot_block[0x2d] = 'l';
	boot_block[0x2e] = 'u';
	boot_block[0x2f] = 'm';
	boot_block[0x30] = 'e';
	boot_block[0x31] = 0x20;
	boot_block[0x32] = 0x20;
	boot_block[0x33] = 0x20;
	boot_block[0x34] = 0x20;
	boot_block[0x35] = 0x20;

	// File system identifier. (8 bytes)
	boot_block[0x36] = 'F';
	boot_block[0x37] = 'A';
	boot_block[0x38] = 'T';
	boot_block[0x39] = '1';
	boot_block[0x3a] = '2';
	boot_block[0x3b] = 0x20;
	boot_block[0x3c] = 0x20;
	boot_block[0x3d] = 0x20;
}

void kiv_fs::format_disk(const FAT_Version version, void* boot_block, const kiv_hal::TDrive_Parameters& params) {
	switch (version) {
		case kiv_fs::FAT_Version::FAT16: format_fat16(static_cast<unsigned char*>(boot_block), params); break;
	}
}

void kiv_fs::boot_block(FATBoot_Block& boot_block, const uint16_t bytes_per_sector, const void* sector) {
	const auto block = static_cast<const char*>(sector);
	boot_block.description = std::string(&block[0x03], 8);
	boot_block.number_of_fat = block[0x10];
	boot_block.bytes_per_sector = block[0x0c] << 8 | block[0x0b];
	boot_block.blocks_for_fat = block[0x17] << 8 | block[0x16];
	boot_block.number_of_root_directory_entries = block[0x12] << 8 | block[0x11];
	boot_block.reserved_sectors = block[0x0e];
	boot_block.absolute_number_of_sectors = block[0x14] << 8 | block[0x13];
}

void kiv_fs::entire_directory(std::vector<FATEntire_Directory>& entire_directories, const uint16_t bytes_per_sector, void* sector) {
	const auto block = static_cast<char*>(sector);
	for (size_t beg = 0; beg < bytes_per_sector; beg += sizeof(FATEntire_Directory)) {
		if (block[beg] == 0x00) break;
		entire_directories.push_back(*reinterpret_cast<FATEntire_Directory*>(block + beg));
	}
}

bool kiv_fs::is_formatted(const void* sector) {
	const auto block = static_cast<const char*>(sector);
	return block[0] == '.' && block[1] == '<' && block[2] == '.';
}

std::vector<size_t> kiv_fs::sectors_for_root_dir(const FATBoot_Block& boot_block) {

	const auto address = root_directory_addr(boot_block);
	const auto size = root_directory_size(boot_block);

	std::vector<size_t> sectors(size);
	std::iota(sectors.begin(), sectors.end(), address);

	return sectors;
}

uint16_t kiv_fs::root_directory_addr(const FATBoot_Block& boot_block) {
	return boot_block.number_of_fat * boot_block.blocks_for_fat + 1;
}

uint8_t kiv_fs::root_directory_size(const FATBoot_Block& boot_block) {
	return static_cast<uint8_t>(boot_block.number_of_root_directory_entries * sizeof(kiv_fs::FATEntire_Directory) / boot_block.bytes_per_sector);
}
