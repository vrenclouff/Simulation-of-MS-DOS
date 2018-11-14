#include "iohandle.h"

#include "../api/hal.h"
#include "fat_tools.h"

size_t iohandle::console::write(char * buffer, size_t buffer_size) {
	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(buffer);
	regs.rcx.r = buffer_size;
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, regs);
	return regs.rcx.r;
}

size_t iohandle::console::read(char * buffer, const size_t buffer_size) {
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
			case kiv_hal::NControl_Codes::NUL:			//chyba cteni? nebo ctrl+z -> posli signal pro ukonceni
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

size_t iohandle::sys::procfs(char * buffer, const size_t buffer_size) {

	const auto head = "PID \t PPID \t STIME \t COMMAND \n";

	const auto size = strlen(head) > buffer_size ? buffer_size : strlen(head);
	std::copy(head, head + size, buffer);

	return strlen(head);
}

size_t iohandle::file::write(char * buffer, size_t buffer_size) {
	return size_t();
}

size_t iohandle::file::read(const kiv_fs::Drive_Desc& drive, const kiv_fs::FATEntire_Directory& entire_dir, char* buffer, const size_t buffer_size, size_t& seek) {

	std::vector<size_t> sectors;	
	if (entire_dir.first_cluster != kiv_fs::root_directory_addr(drive.boot_block)) {
		sectors = kiv_fs::sectors_for_entire_dir(entire_dir, drive.boot_block.bytes_per_sector, kiv_fs::offset(drive.boot_block));
	}
	else {
		sectors = kiv_fs::sectors_for_root_dir(drive.boot_block);
	}

	std::vector<unsigned char> arr(drive.boot_block.bytes_per_sector);
	kiv_hal::TDisk_Address_Packet dap;
	dap.sectors = static_cast<void*>(arr.data());
	dap.count = 1;
	dap.lba_index = sectors.front();

	kiv_hal::TRegisters regs;
	regs.rdx.l = drive.id;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
	regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);

	if (fat_tool::is_attr(entire_dir.attributes, kiv_os::NFile_Attributes::Directory)) {

		auto& read_entries = seek;

		const auto entries_per_sector = drive.boot_block.bytes_per_sector / sizeof(kiv_fs::FATEntire_Directory);
		const auto entries_for_buffer = buffer_size / sizeof(kiv_os::TDir_Entry);

		const auto start_sector = read_entries <= 0 ? 0 : read_entries / entries_per_sector;
		const auto start_entry = read_entries <= 0 ? 0 : read_entries - (start_sector * entries_per_sector);

		size_t offset = 0;
		for (auto sector = sectors.begin() + start_sector; sector != sectors.end(); sector++) {
			dap.lba_index = *sector;
			
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
			
			std::vector<kiv_fs::FATEntire_Directory> entire_dirs;
			kiv_fs::entire_directory(entire_dirs, drive.boot_block.bytes_per_sector, arr.data());

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
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);
		const auto size = arr.size() < buffer_size ? arr.size() : buffer_size;
		memcpy(buffer, arr.data(), size);
		return size;
	}
}