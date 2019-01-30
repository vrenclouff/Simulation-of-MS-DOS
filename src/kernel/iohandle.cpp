#include "iohandle.h"

#include "../api/hal.h"
#include "fat_tools.h"
#include "common.h"

// @author: Lukas Cerny

size_t IOHandle_VGA::write(char* buffer, const size_t buffer_size, kiv_os::NOS_Error& error) {
	if (!IOHandle::check_ACL(Permission::Write)) {
		error = kiv_os::NOS_Error::Permission_Denied;
		return 0;
	}

	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(buffer);
	regs.rcx.r = buffer_size;
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, regs);
	return regs.rcx.x;
}

size_t IOHandle_Keyboard::read(char* buffer, const size_t buffer_size, kiv_os::NOS_Error& error) {
	if (!IOHandle::check_ACL(Permission::Read)) {
		error = kiv_os::NOS_Error::Permission_Denied;
		return 0;
	}

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
		case kiv_hal::NControl_Codes::NUL: {	//chyba cteni? nebo ctrl+z -> posli signal pro ukonceni
			return 0;
		}
			
		case kiv_hal::NControl_Codes::CR: {	//docetli jsme az po Enter
			char linebreak = '\n';
			buffer[pos] = linebreak;
			pos++;
			registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_String);
			registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(&linebreak);
			registers.rcx.r = 1;
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
			return pos;
			break;
		}

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

size_t IOHandle_File::write(char * buffer, const size_t buffer_size, kiv_os::NOS_Error& error) {
	if(!IOHandle::check_ACL(Permission::Write)) {
		error = kiv_os::NOS_Error::Permission_Denied;
		return 0;
	}

	std::lock_guard<std::mutex> locker(_io_mutex);

	auto disk_status = kiv_hal::NDisk_Status::No_Error;

	const auto& boot_block = _drive.boot_block;
	const auto bytes_per_sector = _drive.boot_block.bytes_per_sector;
	const auto offset = kiv_fs::offset(boot_block);

	auto div = std::div(static_cast<int>(buffer_size), bytes_per_sector);
	uint16_t size_in_blocks = (div.rem ? div.quot + 1 : div.quot); // TODO 

	std::vector<std::div_t> allocated_space;
	const auto last_sector = _file.sectors.back();
	_file.sectors.pop_back();

	kiv_fs::sector_to_fat_offset(allocated_space, std::vector<uint16_t>{ last_sector }, bytes_per_sector, offset);

	auto last_offset = allocated_space.front();
	last_offset.rem += 2;

	if (size_in_blocks > allocated_space.size()) {
		if (!kiv_fs::find_free_sectors(allocated_space, _drive.id, last_offset, size_in_blocks, _drive.boot_block.bytes_per_sector, disk_status)) {
			error = kiv_os::NOS_Error::Not_Enough_Disk_Space;
			return 0;
		}
	}

	kiv_hal::TDisk_Address_Packet dap;
	dap.count = 1;
	kiv_hal::TRegisters regs;
	regs.rdx.l = _drive.id;
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Write_Sectors);
	regs.flags.carry = 0;

	auto find_sector = [&](const std::div_t& ofst) {
		return ((((ofst.quot - 1) * bytes_per_sector) + ofst.rem) / MULTIPLY_CONST);
	};

	uint16_t buffer_seek = 0;
	auto data = std::vector<char>(buffer, buffer + buffer_size);
	for (const auto& fat_offset : allocated_space) {
		const auto sector = find_sector(fat_offset) + offset;
		_file.sectors.push_back(sector);

		if (buffer_size - buffer_seek < bytes_per_sector) {
			data.resize((size_t)size_in_blocks * (size_t)bytes_per_sector);
		}

		dap.sectors = static_cast<void*>(data.data() + buffer_seek);
		dap.lba_index = sector;
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

		if (regs.flags.carry) {
			disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
			return 0;
		}

		buffer_seek += bytes_per_sector;
	}

	seek += buffer_size;

	uint16_t first_sector;
	std::vector<uint16_t> sectors;

	if (allocated_space.size() > 1) {
		if (!kiv_fs::save_to_fat(_drive.id, allocated_space, bytes_per_sector, offset, sectors, first_sector, disk_status)) {
			error = kiv_os::NOS_Error::IO_Error;
			return 0;
		}
	}

	_file.entire_dir.size = static_cast<uint32_t>(seek);

	if (!kiv_fs::save_to_dir(_drive.id, _parrent_sectors, bytes_per_sector, _file.entire_dir, kiv_fs::Edit_Type::Edit, disk_status)) {
		error = kiv_os::NOS_Error::IO_Error;
		return 0;
	}


	return buffer_size;
}

size_t IOHandle_File::read(char* buffer, const size_t buffer_size, kiv_os::NOS_Error& error) {
	if (!IOHandle::check_ACL(Permission::Read)) {
		error = kiv_os::NOS_Error::Permission_Denied;
		return 0;
	}

	std::lock_guard<std::mutex> locker(_io_mutex);

	auto disk_status = kiv_hal::NDisk_Status::No_Error;

	const auto bytes_per_sector = _drive.boot_block.bytes_per_sector;

	const auto& sectors = _file.sectors;
	const auto& entire_dir = _file.entire_dir;

	std::vector<unsigned char> arr(bytes_per_sector);
	kiv_hal::TDisk_Address_Packet dap;
	dap.sectors = static_cast<void*>(arr.data());
	dap.count = 1;

	kiv_hal::TRegisters regs;
	regs.rdx.l = _drive.id;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);
	regs.flags.carry = 0;

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
			
			if (regs.flags.carry) {
				disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
				return 0;
			}

			std::vector<kiv_fs::FATEntire_Directory> entire_dirs;
			kiv_fs::entire_directory(entire_dirs, bytes_per_sector, arr.data());

			if (start_entry >= entire_dirs.size()) {
				return offset;
			}

			for (auto item = entire_dirs.begin() + start_entry; item != entire_dirs.end(); item++) {
				const auto& dir = *item;
				read_entries++;

				memcpy(buffer + offset, &dir.attributes, sizeof kiv_os::TDir_Entry::file_attributes);
				offset += sizeof kiv_os::TDir_Entry::file_attributes;

				auto file_name = fat_tool::rtrim(std::string(dir.file_name, sizeof kiv_fs::FATEntire_Directory::file_name));
				if (!fat_tool::is_attr(dir.attributes, kiv_os::NFile_Attributes::Directory) && strlen(dir.extension)) {
					file_name.append(".").append(fat_tool::rtrim(std::string(dir.extension, sizeof kiv_fs::FATEntire_Directory::extension)));
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
			
			if (regs.flags.carry) {
				disk_status = static_cast<kiv_hal::NDisk_Status>(regs.rax.x);
				return 0;
			}

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

size_t procfs(char* buffer, const size_t buffer_size) {
	const auto result = process_manager->get_process_table();
	const auto size = result.size() > buffer_size ? buffer_size : result.size();
	const auto str = result.data();
	std::copy(str, str + size, buffer);
	return size;
}

size_t IOHandle_SYS::read(char* buffer, const size_t buffer_size, kiv_os::NOS_Error& error) {
	if (!IOHandle::check_ACL(Permission::Read)) {
		error = kiv_os::NOS_Error::Permission_Denied;
		return 0;
	}

	std::lock_guard<std::mutex> locker(_io_mutex);

	switch (_type) {
		case SYS_Type::PROCFS: return procfs(buffer, buffer_size);
		default: return 0;
	}
}

size_t IOHandle_Pipe::write(char* buffer, const size_t buffer_size, kiv_os::NOS_Error& error) {
	if (!IOHandle::check_ACL(Permission::Write)) {
		error = kiv_os::NOS_Error::Permission_Denied;
		return 0;
	}

	std::lock_guard<std::mutex> locker(_io_mutex);

	size_t written = 0;
	for (; written < buffer_size; written++) {
		_circular_buffer->write(buffer[written]);
	}

	return written;
}

size_t IOHandle_Pipe::read(char* buffer, const size_t buffer_size, kiv_os::NOS_Error& error) {
	if (!IOHandle::check_ACL(Permission::Read)) {
		error = kiv_os::NOS_Error::Permission_Denied;
		return 0;
	}

	std::lock_guard<std::mutex> locker(_io_mutex);

	size_t read = 0;
	for (; read < buffer_size; read++) {
		auto val = _circular_buffer->read();
		if (val == -1) break;
		buffer[read] = val;
	}

	return read;
}

void IOHandle_Pipe::close() {
	_circular_buffer->close();
}