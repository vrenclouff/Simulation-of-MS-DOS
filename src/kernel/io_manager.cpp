#include "io_manager.h"

#include "fat_tools.h"
#include "fat_file.h"

bool find_entire_dir(kiv_fs::FATEntire_Directory& entire_file, const uint8_t drive_id, const std::vector<std::string> components, const std::vector<size_t> init_sectors, const uint16_t bytes_per_sector) {
	size_t deep = 0;
	bool founded;
	std::vector<unsigned char> arr(bytes_per_sector);
	do {
		for (auto& sector : init_sectors) {

			kiv_hal::TDisk_Address_Packet dap;
			dap.sectors = static_cast<void*>(arr.data());
			dap.count = 1;
			dap.lba_index = sector;

			kiv_hal::TRegisters regs;
			regs.rdx.l = drive_id;
			regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NDisk_IO::Read_Sectors);
			regs.rdi.r = reinterpret_cast<decltype(regs.rdi.r)>(&dap);

			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Disk_IO, regs);

			std::vector<kiv_fs::FATEntire_Directory> entries;
			kiv_fs::entire_directory(entries, bytes_per_sector, arr.data());

			const auto finded_element = fat_tool::parse_entire_name(components.at(++deep));
			const auto is_dir = finded_element.extension.empty();
			founded = false;
			for (auto& entire : entries) {
				// pokud hledam slozku a slozka to neni, prejdi na dalsi
				const auto is_entire_dir = fat_tool::is_attr(entire.attributes, kiv_os::NFile_Attributes::Directory);
				if (is_dir && !is_entire_dir) {
					continue;
				}

				// porovnej jmena a zjisti, zda hledam prave tento 'soubor'
				const auto name = fat_tool::rtrim(std::string(entire.file_name, sizeof(entire.file_name)));
				if (finded_element.name.compare(name) == 0) {

					// pokud hledam slozku, porovnej nazvy a shoduji-li se, uloz vstup
					if (is_entire_dir) {
						entire_file = entire;
						founded = true; break;
					}

					// pokud hledam soubor, porovnej konzovky a shoduji-li se, uloz vstup
					const auto extension = fat_tool::rtrim(std::string(entire.extension, sizeof(entire.extension)));
					if (finded_element.extension.compare(extension) == 0) {
						entire_file = entire;
						founded = true; break;
					}
				}
			}
			if (founded) break;
		}
	} while (deep < components.size() - 1);

	return founded;
}

bool IOManager::open(const std::string drive_volume, const std::vector<std::string> path_components) {

	const auto drive = registred_drivers[drive_volume];
	const auto sectors = kiv_fs::sectors_for_root_dir(drive.boot_block);

	kiv_fs::FATEntire_Directory entire_file;
	if (find_entire_dir(entire_file, drive.id, path_components, sectors, drive.boot_block.bytes_per_sector)) {
		// TODO entire file wasn't found
	}
 
	// TODO load all sectors for entire file

	return false;
}

bool IOManager::register_drive(const std::string volume, const uint8_t id, const kiv_fs::FATBoot_Block& book_block) {
	if (registred_drivers.find(volume) != registred_drivers.end()) {
		return false;
	}
	registred_drivers[volume] = { id, book_block };
	return true;
}