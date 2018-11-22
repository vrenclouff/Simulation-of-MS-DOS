#include "io.h"
#include "kernel.h"
#include "handles.h"
#include "pipe.h"

#include "fat_tools.h"
#include "io_manager.h"
#include "common.h"

#include <filesystem>
#include <functional>
#include <vector>

namespace fs = std::filesystem;

std::map<std::string, kiv_fs::Drive_Desc> registred_drivers;


STDHandle Register_STD() {
	auto console = new IOHandle_Console();
	const auto  in = Convert_Native_Handle(static_cast<HANDLE>(console));
	const auto out = Convert_Native_Handle(static_cast<HANDLE>(console));
	return { in, out };
}

std::string to_absolute_path(std::string base_path, std::string path) {
	if (fs::u8path(path).is_absolute()) {
		return path;
	}
	auto fsbasepath = fs::u8path(base_path);
	fsbasepath /= path;
	return fsbasepath.lexically_normal().string();
}

void to_absolute_components(std::vector<std::string>& components, std::string path) {
	for (const auto& item : std::filesystem::path(path)) {
		components.push_back(item.string());
	}
	if (components.back().empty()) {
		components.pop_back();
	}
}

bool is_exist_dir(std::string path) {
	std::vector<std::string> components;
	to_absolute_components(components, path);
	kiv_fs::Drive_Desc drive = registred_drivers[components.front()];
	components.erase(components.begin());
	kiv_fs::FATEntire_Directory entry;
	const auto res = kiv_fs::find_entire_dir(entry, components, drive);
	return res && fat_tool::is_attr(entry.attributes, kiv_os::NFile_Attributes::Directory);
}

IOHandle* Open_File(std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes) {

	std::vector<std::string> components;
	to_absolute_components(components, absolute_path);

	const auto drive_volume = components.front();
	components.erase(components.begin());

	if (fm == kiv_os::NOpen_File::fmOpen_Always) {
		if (drive_volume.compare("A:") == 0) {
			std::string sys;
			for (const auto& cmp : components) {
				sys += cmp;
			}
			return SYS_HANDLERS.at(sys);
		}
		else {
			kiv_fs::Drive_Desc drive = registred_drivers[drive_volume];
			kiv_fs::FATEntire_Directory entry;
			if (!kiv_fs::find_entire_dir(entry, components, drive)) {
				// TODO error -> entire_dir wasn't found -> file doesn't exist
				return nullptr;
			}
			const std::vector<uint16_t> sectors = kiv_fs::load_sectors(drive, entry);
			kiv_fs::File_Desc file = { entry, sectors };
			return new IOHandle_File(drive, file);
		}
	}
	else {
		
		const auto filename = fat_tool::parse_entire_name(components.back());
		components.pop_back();

		if (filename.name.size() > sizeof kiv_fs::FATEntire_Directory::file_name ) {
			// TODO exception -> filename is larger than is allowed
			return nullptr;
		}

		if (filename.extension.size() > sizeof kiv_fs::FATEntire_Directory::extension) {
			// TODO exception -> extension is larger than is allowed
			return nullptr;
		}

		kiv_fs::FATEntire_Directory entry;
		std::copy(&(filename.name)[0], &(filename.name)[0] + filename.name.size(), entry.file_name);
		entry.file_name[filename.name.size()] = 0;

		const auto is_dir = fat_tool::is_attr(static_cast<uint8_t>(attributes), kiv_os::NFile_Attributes::Directory);

		if (is_dir) {
			entry.extension[0] = 0;
		}
		else {
			std::copy(&filename.extension[0], &filename.extension[0] + filename.extension.size(), entry.extension);
		}

		entry.attributes = static_cast<decltype(entry.attributes)>(attributes);
		entry.size = 0;

		//time_t now = time(0);
		//std::tm ltm = *localtime(&now);

		entry.date = 0; // fat_tool::to_date(ltm);
		entry.time = 0; // fat_tool::to_time(ltm);

		kiv_fs::Drive_Desc drive = registred_drivers[drive_volume];
		kiv_fs::File_Desc file = { entry, std::vector<uint16_t>() };

		// najdi vstupni adresar, kam se ma 'soubor' ulozit
		kiv_fs::FATEntire_Directory parrent_entry;
		if (!kiv_fs::find_entire_dir(parrent_entry, components, drive)) {
			// TODO error -> entire_dir wasn't found -> file doesn't exist
			return nullptr;
		}

		if (!fat_tool::is_attr(parrent_entry.attributes, kiv_os::NFile_Attributes::Directory)) {
			// TODO error -> folder that we want to create new file/dir isn't exist
			return nullptr;
		}

		if (components.size() > 1) {
			components.pop_back();

			kiv_fs::FATEntire_Directory grandparent_entry;
			if (!kiv_fs::find_entire_dir(grandparent_entry, components, drive)) {
				// TODO error
				return nullptr;
			}

			parrent_entry.size += sizeof kiv_fs::FATEntire_Directory;

			const auto grandparent_sectors = kiv_fs::load_sectors(drive, grandparent_entry);
			if (!kiv_fs::save_to_dir(drive.id, grandparent_sectors, drive.boot_block.bytes_per_sector, parrent_entry, kiv_fs::Edit_Type::Edit)) {
				// TODO error
				return nullptr;
			}
		}

		const auto sectors = kiv_fs::load_sectors(drive, parrent_entry);
		if (!kiv_fs::create_dir(drive, kiv_fs::File_Desc{ parrent_entry, sectors }, file)) {
			// TODO error -> cannot create new dir
		}

		return new IOHandle();
	}
}

bool Remove_File(std::string absolute_path) {

	std::vector<std::string> components;
	to_absolute_components(components, absolute_path);


	kiv_fs::Drive_Desc drive = registred_drivers[components.front()];
	components.erase(components.begin());

	kiv_fs::FATEntire_Directory entry_to_dell;
	if (!kiv_fs::find_entire_dir(entry_to_dell, components, drive)) {
		// TODO error -> entire_dir wasn't found -> the dir doesn't exist
		kiv_os::NOS_Error::File_Not_Found;
		return false;
	}

	if (!fat_tool::is_attr(entry_to_dell.attributes, kiv_os::NFile_Attributes::Directory)) {
		// TODO error -> isn't the dir
		kiv_os::NOS_Error::File_Not_Found;
		return false;
	}

	if (entry_to_dell.size) {
		// TODO error -> can delete only free dirs
		kiv_os::NOS_Error::Directory_Not_Empty;
		return false;
	}


	// free space in FAT
	kiv_fs::remove_sectors_in_fat(entry_to_dell, drive.boot_block.bytes_per_sector, drive.id);

	// find parrent to remove Entry_dir
	components.pop_back();
	kiv_fs::FATEntire_Directory parrent_entry;
	if (!kiv_fs::find_entire_dir(parrent_entry, components, drive)) {
		// TODO error -> entire_dir wasn't found -> file doesn't exist
		return false;
	}

	const auto parrent_sectors = kiv_fs::load_sectors(drive, parrent_entry);
	if (!kiv_fs::save_to_dir(drive.id, parrent_sectors, drive.boot_block.bytes_per_sector, entry_to_dell, kiv_fs::Edit_Type::Del)) {
		// TODO error -> cannot delete entry_dir from cluster
		return false;
	}

	if (components.size() > 1) {
		// find grandparent

		components.pop_back();
		kiv_fs::FATEntire_Directory grandparrent_entry;
		if (!kiv_fs::find_entire_dir(grandparrent_entry, components, drive)) {
			// TODO error -> entire_dir wasn't found -> file doesn't exist
			kiv_os::NOS_Error::File_Not_Found;;
			return false;
		}

		parrent_entry.size -= sizeof kiv_fs::FATEntire_Directory;

		const auto grandparrent_sectors = kiv_fs::load_sectors(drive, grandparrent_entry);
		if (!kiv_fs::save_to_dir(drive.id, grandparrent_sectors, drive.boot_block.bytes_per_sector, parrent_entry, kiv_fs::Edit_Type::Edit)) {
			// TODO error
			return false;
		}
	}

	return true;
}

void Handle_IO(kiv_hal::TRegisters &regs) {

	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {

		case kiv_os::NOS_File_System::Open_File: {
			const auto path = reinterpret_cast<char*>(regs.rdx.r);
			const auto fm = static_cast<kiv_os::NOpen_File>(regs.rcx.l);
			const auto attributes = static_cast<kiv_os::NFile_Attributes>(regs.rdi.i);
			const auto source = Open_File(path, fm, attributes);
			regs.rax.x = Convert_Native_Handle(static_cast<HANDLE>(source));
		} break;

		case kiv_os::NOS_File_System::Read_File: {
			const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
			regs.rax.r = source->read(reinterpret_cast<char*>(regs.rdi.r), regs.rcx.r);
		} break;

		case kiv_os::NOS_File_System::Write_File: {
			const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
			regs.rax.r = source->write(reinterpret_cast<char*>(regs.rdi.r), regs.rcx.r);
			regs.flags.carry |= (regs.rax.r == 0 ? 1 : 0);
		} break;

		case kiv_os::NOS_File_System::Delete_File: {
			auto path = std::string(reinterpret_cast<char*>(regs.rdx.r));
			const auto process = process_manager->getRunningProcess();
			regs.flags.carry = !Remove_File(to_absolute_path(process->working_dir, path));

		} break;

		case kiv_os::NOS_File_System::Close_Handle: {
			const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
			regs.flags.carry |= !source->close();
			regs.flags.carry |= !Remove_Handle(regs.rdx.x);
			delete source;
		} break;

		case kiv_os::NOS_File_System::Get_Working_Dir: {
			auto path_buffer = reinterpret_cast<char*>(regs.rdx.r);
			const auto buffer_size = regs.rcx.r;
			const auto process = process_manager->getRunningProcess();
			const auto working_dir = std::string_view(process->working_dir);
			const size_t size = buffer_size <= working_dir.size() ? buffer_size : working_dir.size();
			std::copy(&working_dir[0], &working_dir[0] + size, path_buffer);
			regs.rax.r = size;
		} break;

		case kiv_os::NOS_File_System::Set_Working_Dir: {
			const auto path = std::string(reinterpret_cast<char*>(regs.rdx.r));
			const auto process = process_manager->getRunningProcess();
			const auto full_path = to_absolute_path(process->working_dir, path);

			if (is_exist_dir(full_path)) {
				process->working_dir = full_path;
			}
			else {
				regs.flags.carry = 1;
				regs.rax.x = static_cast<decltype(regs.rax.x)>(kiv_os::NOS_Error::File_Not_Found);
			}
		} break;

		case kiv_os::NOS_File_System::Create_Pipe: {
			Pipe* pipe = new Pipe();
			kiv_os::THandle handles[2];

			handles[0] = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_Pipe(pipe)));
			handles[1] = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_Pipe(pipe)));
			regs.rbx.r = reinterpret_cast<decltype(regs.rbx.r)>(handles);
		} break;
	}
}

std::string io::main_drive() {
	return registred_drivers.begin()->first;
}

bool io::register_drive(const std::string volume, const uint8_t id, const kiv_fs::FATBoot_Block& book_block) {
	if (registred_drivers.find(volume) != registred_drivers.end()) { return false; }
	registred_drivers[volume] = { id, book_block };
	return true;
}


/* Nasledujici dve vetve jsou ukazka, ze starsiho zadani, ktere ukazuji, jak mate mapovat Windows HANDLE na kiv_os handle a zpet, vcetne jejich alokace a uvolneni

	case kiv_os::scCreate_File: {
		HANDLE result = CreateFileA((char*)regs.rdx.r, GENERIC_READ | GENERIC_WRITE, (DWORD)regs.rcx.r, 0, OPEN_EXISTING, 0, 0);
		//zde je treba podle Rxc doresit shared_read, shared_write, OPEN_EXISING, etc. podle potreby
		regs.flags.carry = result == INVALID_HANDLE_VALUE;
		if (!regs.flags.carry) regs.rax.x = Convert_Native_Handle(result);
		else regs.rax.r = GetLastError();
	}
								break;	//scCreateFile

	case kiv_os::scClose_Handle: {
			HANDLE hnd = Resolve_kiv_os_Handle(regs.rdx.x);
			regs.flags.carry = !CloseHandle(hnd);
			if (!regs.flags.carry) Remove_Handle(regs.rdx.x);
				else regs.rax.r = GetLastError();
		}

		break;	//CloseFile

*/
