#include "fat.h"

#include "fat_tools.h"

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
	boot_block[0x3a] = '6';
	boot_block[0x3b] = 0x20;
	boot_block[0x3c] = 0x20;
	boot_block[0x3d] = 0x20;
}

bool kiv_fs::new_entire_dir(kiv_fs::FATEntire_Directory & entry, const std::string name, const uint8_t attributes, kiv_hal::NDisk_Status& error) {

	const auto filename = fat_tool::parse_entire_name(name);

	if (filename.name.size() > sizeof kiv_fs::FATEntire_Directory::file_name) {
		error = kiv_hal::NDisk_Status::Bad_Command;
		return false;
	}

	if (filename.extension.size() > sizeof kiv_fs::FATEntire_Directory::extension) {
		error = kiv_hal::NDisk_Status::Bad_Command;
		return false;
	}

	std::copy(&(filename.name)[0], &(filename.name)[0] + filename.name.size(), entry.file_name);
	entry.file_name[filename.name.size()] = 0;

	const auto is_dir = fat_tool::is_attr(attributes, kiv_os::NFile_Attributes::Directory);

	if (is_dir || filename.extension.empty()) {
		entry.extension[0] = 0;
	}
	else {
		std::copy(&filename.extension[0], &filename.extension[0] + filename.extension.size(), entry.extension);
	}

	entry.attributes = attributes;
	entry.size = 0;

	entry.date = 0; // fat_tool::to_date(tm);
	entry.time = 0; // fat_tool::to_time(tm);

	return true;
}

void kiv_fs::format_disk(const FAT_Version version, void* boot_block, const kiv_hal::TDrive_Parameters& params) {
	switch (version) {
		case kiv_fs::FAT_Version::FAT16: format_fat16(static_cast<unsigned char*>(boot_block), params); break;
	}
}

void kiv_fs::boot_block(FATBoot_Block& boot_block, const size_t bytes_per_sector, const void* sector) {
	const auto block = static_cast<const char*>(sector);
	//boot_block.description = std::string(&block[0x03], 8);
	boot_block.bytes_per_sector = block[0x0c] << 8 | block[0x0b];
	boot_block.number_of_fat = block[0x10];
	boot_block.blocks_for_fat = block[0x17] << 8 | block[0x16];
	boot_block.number_of_root_directory_entries = block[0x12] << 8 | block[0x11];
	// boot_block.reserved_sectors = block[0x0e];
	// boot_block.absolute_number_of_sectors = block[0x14] << 8 | block[0x13];
}

void kiv_fs::entire_directory(std::vector<FATEntire_Directory>& entire_directories, const size_t bytes_per_sector, void* sector) {
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

uint16_t kiv_fs::offset(const FATBoot_Block & boot_block) {
	const auto root_dir_addr = kiv_fs::root_directory_addr(boot_block);
	const auto number_of_blocks = kiv_fs::root_directory_size(boot_block);
	return root_dir_addr + number_of_blocks - 0x2;
}

std::vector<uint16_t> sectors_for_root_dir(const kiv_fs::FATBoot_Block& boot_block) {
	const auto address = root_directory_addr(boot_block);
	const auto size = root_directory_size(boot_block);
	std::vector<uint16_t> sectors(size);
	std::iota(sectors.begin(), sectors.end(), address);
	return sectors;
}

void kiv_fs::remove_sectors_in_fat(const kiv_fs::FATEntire_Directory& entire_dir, const size_t bytes_per_sector, const uint8_t drive_id, kiv_hal::NDisk_Status& disk_status) {

	std::vector<unsigned char> fat(bytes_per_sector);
	std::div_t fat_offset, next_fat_offset;
	uint16_t fake_sector, next_fake_sector;

	fake_sector = entire_dir.first_cluster;
	next_fake_sector = { 0 }; next_fat_offset = { 0 };

	kiv_hal::TDisk_Address_Packet dap;
	dap.sectors = static_cast<void*>(fat.data());
	dap.count = 1;

	kiv_hal::TRegisters regs;
	regs.rdx.l = drive_id;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
	regs.flags.carry = 0;

	fat_offset = std::div(fake_sector * MULTIPLY_CONST, static_cast<int>(bytes_per_sector));
	fat_offset.quot += 1;

	dap.lba_index = fat_offset.quot;
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
	
	if (regs.flags.carry) {
		disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
		return;
	}

	next_fake_sector = (uint16_t)fat.at((size_t)fat_offset.rem) | (uint16_t)fat.at((size_t)fat_offset.rem + 1) << 8;
	fat[(size_t)fat_offset.rem] = fat.at((size_t)fat_offset.rem + 1) = kiv_fs::FAT_Attributes::Free;

	while (next_fake_sector != kiv_fs::FAT_Attributes::End) {
		// TODO for files
	}

	dap.lba_index = fat_offset.quot;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
	if (regs.flags.carry) {
		disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
	}
}

void kiv_fs::sector_to_fat_offset(std::vector<std::div_t>& fat_offsets, const std::vector<uint16_t> sectors, const size_t bytes_per_sector, const uint16_t offset) {
	for (const auto& sector : sectors) {
		auto fat_offset = std::div((sector - offset) * MULTIPLY_CONST, static_cast<int>(bytes_per_sector));
		fat_offset.quot += 1;
		fat_offsets.push_back(fat_offset);
	}
}

std::vector<uint16_t> sectors_for_entire_dir(const kiv_fs::FATEntire_Directory& entire_dir, const size_t bytes_per_sector, const uint16_t offset, const uint8_t drive_id) {
	std::vector<unsigned char> fat(bytes_per_sector);
	std::div_t fat_offset, next_fat_offset;
	uint16_t fake_sector, next_fake_sector;

	fake_sector = entire_dir.first_cluster;
	next_fake_sector = { 0 }; next_fat_offset = { 0 };

	auto sectors = std::vector<uint16_t>();

	do {
		sectors.push_back(fake_sector + offset);

		fat_offset = std::div(fake_sector * MULTIPLY_CONST, static_cast<int>(bytes_per_sector));
		fat_offset.quot += 1;

		if (fat_offset.quot != next_fat_offset.quot) {

			kiv_hal::TDisk_Address_Packet dap;
			dap.sectors = static_cast<void*>(fat.data());
			dap.count = 1;
			dap.lba_index = fat_offset.quot;

			kiv_hal::TRegisters regs;
			regs.rdx.l = drive_id;
			regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
			regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
			regs.flags.carry = 0;

			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			// TODO

			next_fat_offset = fat_offset;
		}

		next_fake_sector = fat[(size_t)fat_offset.rem] |fat[(size_t)fat_offset.rem + 1] << 8;
		fake_sector = next_fake_sector;

		if (next_fake_sector == 0) {
			return std::vector<uint16_t>();
		}

	} while (next_fake_sector != kiv_fs::FAT_Attributes::End);

	return sectors;
}

bool kiv_fs::find_entire_dirs(const kiv_fs::Drive_Desc drive, std::vector<kiv_fs::File_Desc>& files, std::vector<std::string> components) {

	components.erase(components.begin());
	kiv_fs::FATEntire_Directory root;
	root.first_cluster = kiv_fs::root_directory_addr(drive.boot_block);
	root.attributes = static_cast<decltype(root.attributes)>(kiv_os::NFile_Attributes::Directory);
	const auto root_sectors = sectors_for_root_dir(drive.boot_block);

	files.push_back({ root, root_sectors});

	if (components.empty()) return true;

	const auto bytes_per_sector = drive.boot_block.bytes_per_sector;
	const auto fat_offset = kiv_fs::offset(drive.boot_block);

	bool founded;
	std::vector<unsigned char> arr(bytes_per_sector);
	auto sectors = root_sectors;
	kiv_fs::FATEntire_Directory actual_entry;
	do {
		founded = false;
		const auto finded_element = fat_tool::parse_entire_name(components.front());
		components.erase(components.begin());

		for (auto& sector : sectors) {

			kiv_hal::TDisk_Address_Packet dap;
			dap.sectors = static_cast<void*>(arr.data());
			dap.count = 1;
			dap.lba_index = sector;

			kiv_hal::TRegisters regs;
			regs.rdx.l = drive.id;
			regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
			regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
			regs.flags.carry = 0;

			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			
			if (regs.flags.carry) {
				return false;
			}

			std::vector<kiv_fs::FATEntire_Directory> entries;
			kiv_fs::entire_directory(entries, bytes_per_sector, arr.data());

			if (entries.empty()) break;

			founded = false;
			for (auto& entire : entries) {
				const auto name = fat_tool::rtrim(std::string(entire.file_name, sizeof(entire.file_name)));
				if (finded_element.name.compare(name) == 0) {
					const auto extension = fat_tool::rtrim(std::string(entire.extension, sizeof(entire.extension)));
					if (finded_element.extension.compare(extension) == 0) {
						actual_entry = entire;
						founded = true; break;
					}
				}
			}
			if (founded) break;
		}

		if (founded) {
			sectors = sectors_for_entire_dir(actual_entry, bytes_per_sector, fat_offset, drive.id);
			files.push_back({ actual_entry, sectors });
		}
		else if (!components.empty()) {
			return false;
		}

	} while (!components.empty());

	return founded;
}

bool kiv_fs::find_free_sectors(std::vector<std::div_t>& fat_offsets, const uint8_t drive_id, const std::div_t sector, const size_t count, const size_t bytes_per_sector, kiv_hal::NDisk_Status& disk_status) {

	std::div_t fat_offset = sector;

	std::vector<unsigned char> fat(bytes_per_sector);
	kiv_hal::TDisk_Address_Packet dap;
	dap.sectors = static_cast<void*>(fat.data());
	dap.count = 1;
	dap.lba_index = fat_offset.quot;

	kiv_hal::TRegisters regs;
	regs.rdx.l = drive_id;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
	regs.flags.carry = 0;

	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
	
	if (regs.flags.carry) {
		disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
		return false;
	}

	do {
		if (fat_offset.rem > bytes_per_sector) {
			fat.resize(fat.size() + bytes_per_sector);

			dap.sectors = static_cast<void*>(fat.data() + bytes_per_sector);
			dap.lba_index = ++fat_offset.quot;
			regs.flags.carry = 0;

			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			
			if (regs.flags.carry) {
				disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
				return false;
			}

			fat_offset.rem = 0;
		}
		uint16_t fat_cell = (uint16_t)fat.at((size_t)fat_offset.rem) | (uint16_t)fat.at((size_t)fat_offset.rem + 1) << 8;
		if (fat_cell == kiv_fs::FAT_Attributes::Free) {
			fat_offsets.push_back(fat_offset);
		}
		fat_offset.rem += sizeof(decltype(fat_cell));
	} while (fat_offsets.size() < count);

	if (fat_offsets.size() < count) {
		return false;
	}

	return true;
}

std::vector<uint16_t> kiv_fs::load_sectors(const kiv_fs::Drive_Desc& drive, const kiv_fs::FATEntire_Directory & entry_dir) {
	const auto boot = drive.boot_block;
	return is_entry_root(boot, entry_dir) ? sectors_for_entire_dir(entry_dir, boot.bytes_per_sector, offset(boot), drive.id) : sectors_for_root_dir(boot);
}

bool kiv_fs::save_to_dir(const uint8_t drive_id, const std::vector<uint16_t> sectors, const size_t bytes_per_sector, const kiv_fs::FATEntire_Directory entire_dir, const kiv_fs::Edit_Type type, kiv_hal::NDisk_Status& disk_status) {

	if (sectors.empty()) {
		return true;
	}

	std::vector<unsigned char> dir(bytes_per_sector);
	kiv_hal::TDisk_Address_Packet dap;
	dap.count = 1;
	dap.sectors = static_cast<void*>(dir.data());

	kiv_hal::TRegisters regs;
	regs.rdx.l = drive_id;
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
	regs.flags.carry = 0;

	const auto max_dir_entries_per_block = static_cast<uint8_t>(bytes_per_sector / sizeof(kiv_fs::FATEntire_Directory));

	auto findEntire = [&](const kiv_fs::FATEntire_Directory& dir) {
		return dir.first_cluster == entire_dir.first_cluster;
	};

	for (const auto& sector : sectors) {

		dap.lba_index = sector;
		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);

		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
		
		if (regs.flags.carry) {
			disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
			return false;
		}

		std::vector<kiv_fs::FATEntire_Directory> dir_entries;
		kiv_fs::entire_directory(dir_entries, bytes_per_sector, dir.data());
		if (dir_entries.size() < max_dir_entries_per_block) {
			switch (type) {
				case kiv_fs::Edit_Type::Add: {
					dir_entries.push_back(entire_dir);
				} break;
				case kiv_fs::Edit_Type::Del: {
					dir_entries.erase(std::remove_if(dir_entries.begin(), dir_entries.end(), findEntire));
				} break;
				case kiv_fs::Edit_Type::Edit: {
					auto dir_entries_itr = std::find_if(dir_entries.begin(), dir_entries.end(), findEntire);
					if (dir_entries_itr != dir_entries.end()) {
						*dir_entries_itr = entire_dir;
					}
				} break;
			}
			
			dir_entries.resize(max_dir_entries_per_block);

			dap.sectors = static_cast<void*>(dir_entries.data());
			dap.lba_index = sector;
			regs.flags.carry = 0;
			regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);

			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			
			if (regs.flags.carry) {
				disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
				return false;
			}

			break;
		}
	}

	return true;
}

bool kiv_fs::save_to_fat(const uint8_t drive_id, const std::vector<std::div_t> fat_offsets, const size_t bytes_per_sector, const uint16_t offset, std::vector<uint16_t>& sectors, uint16_t& first_sector, kiv_hal::NDisk_Status& disk_status) {

	auto find_sector = [&](const std::div_t& ofst) {
		return uint16_t((((((size_t)ofst.quot - 1) * bytes_per_sector) + ofst.rem) / MULTIPLY_CONST));
	};

	auto actual_fat = fat_offsets.front();
	auto fake_sector = find_sector(actual_fat);
	first_sector = fake_sector;
	sectors.push_back(fake_sector + offset);

	std::vector<unsigned char> fat(bytes_per_sector);
	kiv_hal::TDisk_Address_Packet dap;
	dap.sectors = static_cast<void*>(fat.data());
	dap.count = 1;
	dap.lba_index = actual_fat.quot;

	kiv_hal::TRegisters regs;
	regs.rdx.l = drive_id;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
	regs.flags.carry = 0;

	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
	
	if (regs.flags.carry) {
		disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
		return false;
	}

	for (auto next_fat = fat_offsets.begin() + 1; next_fat != fat_offsets.end(); next_fat++) {

		if (dap.lba_index != (*next_fat).quot) {

			regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			
			if (regs.flags.carry) {
				disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
				return false;
			}

			dap.lba_index = (*next_fat).quot;
			regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			
			if (regs.flags.carry) {
				disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
				return false;
			}
		}

		fake_sector = find_sector(*next_fat);
		sectors.push_back(fake_sector + offset);
		auto index = actual_fat.rem;
		fat[index] = fake_sector & 0xff;
		fat.at((size_t)index + 1) = (fake_sector & 0xff00) >> 8;
		actual_fat = *next_fat;
	}
	fat.at((size_t)actual_fat.rem) = fat.at((size_t)actual_fat.rem + 1) = kiv_fs::FAT_Attributes::End >> 8;

	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
	
	if (regs.flags.carry) {
		disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
		return false;
	}

	return true;
}

bool kiv_fs::create_file(const kiv_fs::Drive_Desc& drive, const kiv_fs::File_Desc& parrent, kiv_fs::File_Desc& file) {
	return false;
}

bool kiv_fs::is_entry_root(const kiv_fs::FATBoot_Block& boot, const kiv_fs::FATEntire_Directory& entry) {
	return entry.first_cluster != root_directory_addr(boot);
}

uint16_t kiv_fs::root_directory_addr(const FATBoot_Block& boot_block) {
	return boot_block.number_of_fat * boot_block.blocks_for_fat + 1;
}

uint8_t kiv_fs::root_directory_size(const FATBoot_Block& boot_block) {
	return static_cast<uint8_t>(boot_block.number_of_root_directory_entries * sizeof(kiv_fs::FATEntire_Directory) / boot_block.bytes_per_sector);
}