#include "iohandle.h"

#include "../api/hal.h"
#include "fat_tools.h"
#include "common.h"

size_t IOHandle_Console::write(char * buffer, size_t buffer_size) {
	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(buffer);
	regs.rcx.r = buffer_size;
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, regs);
	return regs.rcx.r;
}

size_t IOHandle_Console::read(char * buffer, size_t buffer_size) {
	kiv_hal::TRegisters registers;

	size_t pos = 0;
	while (pos < buffer_size) {
		//read char
		registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NKeyboard::Read_Char);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, registers);

		char ch = registers.rax.l;

		//osetrime zname kody
		switch (static_cast<kiv_hal::NControl_Codes>(ch)) {
		case kiv_hal::NControl_Codes::BS: {
			//mazeme znak z bufferu
			if (pos <= 0) break;

			pos--;
			registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_Control_Char);
			registers.rdx.l = ch;
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
		} break;

		case kiv_hal::NControl_Codes::LF:  break;	//jenom pohltime, ale necteme
		case kiv_hal::NControl_Codes::NUL:		//chyba cteni? nebo ctrl+z -> posli signal pro ukonceni
		case kiv_hal::NControl_Codes::CR:  return pos;	//docetli jsme az po Enter

		default: buffer[pos] = ch;
			pos++;
			registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_String);
			registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(&ch);
			registers.rcx.r = 1;
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
			break;
		}
	}

	return pos;
}

size_t IOHandle_File::write(char * buffer, size_t buffer_size) {

	const auto boot_block = _drive.boot_block;
	const auto bytes_per_sector = _drive.boot_block.bytes_per_sector;

	auto div = std::div(static_cast<int>(buffer_size), bytes_per_sector);
	uint16_t size_in_blocks = (div.rem ? div.quot + 1 : div.quot);

	std::vector<std::div_t> allocated_space;

	// TODO pouzit pri appendu posledni offset
	std::div_t fat_offset = { 1,  MULTIPLY_CONST * MULTIPLY_CONST };

	std::vector<unsigned char> fat(bytes_per_sector);
	kiv_hal::TDisk_Address_Packet dap;
	dap.sectors = static_cast<void*>(fat.data());
	dap.count = 1;
	dap.lba_index = fat_offset.quot;

	kiv_hal::TRegisters regs;
	regs.rdx.l = _drive.id;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

	// find free sectors for file
	do {
		if (fat_offset.rem > bytes_per_sector) {
			fat.resize(fat.size() + bytes_per_sector);

			dap.sectors = static_cast<void*>(fat.data() + bytes_per_sector);
			dap.lba_index = ++fat_offset.quot;
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

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
		return 0;
	}

	auto find_sector = [&](const std::div_t& ofst) {
		return ((((ofst.quot - 1) * bytes_per_sector) + ofst.rem) / MULTIPLY_CONST);
	};

	// find real clusters for file
	const auto _offset = kiv_fs::offset(boot_block);

	auto actual_fat = allocated_space.at(0);
	auto fake_sector = find_sector(actual_fat);
	_file.sectors.push_back(fake_sector + _offset);

	// set first_cluster for file
	_file.entire_dir.first_cluster = fake_sector;

	for (auto next_fat = allocated_space.begin() + 1; next_fat != allocated_space.end(); next_fat++) {
		fake_sector = find_sector(*next_fat);
		_file.sectors.push_back(fake_sector + _offset);
		auto index = actual_fat.rem;
		fat[index] = fake_sector & 0xff;
		fat[index + 1] = (fake_sector & 0xff00) >> 8;
		actual_fat = *next_fat;
	}

	fat[actual_fat.rem] = fat[actual_fat.rem + 1] = kiv_fs::FAT_Attributes::End >> 8;

	// write allocated fat table to disk
	dap.sectors = static_cast<void*>(fat.data());
	dap.lba_index = actual_fat.quot;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
	//kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

	// write buffer to real clusters
	uint16_t cluster_offset = 0;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
	for (auto& sector : _file.sectors) {
		if (buffer_size - cluster_offset < bytes_per_sector) {
			//_data.resize(size_in_blocks * bytes_per_sector);
		}

		// TODO set real pointer to buffer
		dap.sectors = static_cast<void*>(fat.data() + cluster_offset);
		dap.lba_index = sector;
		//kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		cluster_offset += bytes_per_sector;
	}


	// TODO load sector for parrent
		// move this code to Close_Handle
	const auto parrent_sectors = std::vector<size_t>(); // kiv_fs::load_sectors(NULL);
	const auto max_dir_entries_per_block = static_cast<uint8_t>(bytes_per_sector / sizeof(kiv_fs::FATEntire_Directory));
	std::vector<unsigned char> root_dir(bytes_per_sector);
	for (const auto& sector : parrent_sectors) {

		dap.sectors = static_cast<void*>(root_dir.data());
		dap.lba_index = sector;
		regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		std::vector<kiv_fs::FATEntire_Directory> dir_entries;
		kiv_fs::entire_directory(dir_entries, bytes_per_sector, root_dir.data());
		if (dir_entries.size() < max_dir_entries_per_block) {
			dir_entries.push_back(_file.entire_dir);
			dir_entries.resize(max_dir_entries_per_block);

			dap.sectors = static_cast<void*>(dir_entries.data());
			dap.lba_index = sector;
			regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
			//kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

			break;
		}
	}

	return size_t();
}

size_t IOHandle_File::read(char * buffer, size_t buffer_size) {

	const auto bytes_per_sector = _drive.boot_block.bytes_per_sector;

	const std::vector<size_t> sectors = _file.sectors;
	const kiv_fs::FATEntire_Directory entire_dir = _file.entire_dir;

	std::vector<unsigned char> arr(bytes_per_sector);
	kiv_hal::TDisk_Address_Packet dap;
	dap.sectors = static_cast<void*>(arr.data());
	dap.count = 1;

	kiv_hal::TRegisters regs;
	regs.rdx.l = _drive.id;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);

	if (fat_tool::is_attr(entire_dir.attributes, kiv_os::NFile_Attributes::Directory)) {

		auto& read_entries = seek;

		const auto entries_per_sector = bytes_per_sector / sizeof(kiv_fs::FATEntire_Directory);
		const auto entries_for_buffer = buffer_size / sizeof(kiv_os::TDir_Entry);

		const auto start_sector = read_entries <= 0 ? 0 : read_entries / entries_per_sector;
		const auto start_entry = read_entries <= 0 ? 0 : read_entries - (start_sector * entries_per_sector);

		size_t offset = 0;
		for (auto sector = sectors.begin() + start_sector; sector != sectors.end(); sector++) {
			dap.lba_index = *sector;

			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

			std::vector<kiv_fs::FATEntire_Directory> entire_dirs;
			kiv_fs::entire_directory(entire_dirs, bytes_per_sector, arr.data());

			if (start_entry >= entire_dirs.size()) {
				return offset;
			}

			for (auto dir = entire_dirs.begin() + start_entry; dir != entire_dirs.end(); dir++) {
				read_entries++;

				memcpy(buffer + offset, &(*dir).attributes, sizeof kiv_os::TDir_Entry::file_attributes);
				offset += sizeof kiv_os::TDir_Entry::file_attributes;

				auto file_name = fat_tool::rtrim(std::string((*dir).file_name, sizeof kiv_fs::FATEntire_Directory::file_name));
				if (!fat_tool::is_attr((*dir).attributes, kiv_os::NFile_Attributes::Directory)) {
					file_name.append(".").append(fat_tool::rtrim(std::string((*dir).extension, sizeof kiv_fs::FATEntire_Directory::extension)));
				}
				std::transform(file_name.begin(), file_name.end(), file_name.begin(), ::tolower);
				memcpy(buffer + offset, &file_name[0], file_name.size());
				offset += file_name.size();

				const auto miss = sizeof(kiv_os::TDir_Entry::file_name) - file_name.size();
				for (size_t i = 0; i < miss; i++, offset++) {
					memcpy(buffer + offset, "\0", 1);
				}

				if (read_entries >= entries_for_buffer) {
					return offset;
				}
			}

			if (read_entries >= entries_for_buffer) {
				return offset;
			}
		}
		return offset;
	}
	else {

		auto& read_bytes = seek;

		if (read_bytes >= entire_dir.size) {
			return 0;
		}

		auto read = std::div(static_cast<int>(read_bytes), static_cast<int>(bytes_per_sector));

		size_t offset = 0;
		for (auto sector = sectors.begin() + read.quot; sector != sectors.end(); sector++) {
			dap.lba_index = *sector;

			const auto free_space = buffer_size - offset;
			if (free_space <= 0) break;

			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

			const auto miss_to_copy = entire_dir.size - read_bytes;

			const auto miss_in_sector = bytes_per_sector - read.rem;
			const auto can_to_copy = miss_to_copy > miss_in_sector ? miss_in_sector : miss_to_copy;
			const auto to_copy = free_space > can_to_copy ? can_to_copy : free_space;

			memcpy(buffer + offset, arr.data() + read.rem, to_copy);
			offset += to_copy;
			read_bytes += to_copy;
			read.rem = 0;
		}

		return offset;
	}
}

size_t procfs(char * buffer, const size_t buffer_size) {

	const auto result = process_manager->getProcessTable();
	const auto cresult = result.c_str();
	const auto size = strlen(cresult) > buffer_size ? buffer_size : strlen(cresult);
	std::copy(cresult, cresult + size, buffer);

	return strlen(cresult);
}

size_t IOHandle_SYS::read(char * buffer, size_t buffer_size) {
	switch (_type) {
	case SYS_Type::PROCFS: return procfs(buffer, buffer_size);
	default: return 0;
	}
}