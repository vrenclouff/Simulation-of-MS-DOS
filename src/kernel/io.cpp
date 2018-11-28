#include "io.h"
#include "kernel.h"
#include "handles.h"

#include "fat_tools.h"
#include "io_manager.h"
#include "common.h"

#include <filesystem>
#include <functional>
#include <vector>
#include <ctime>

namespace fs = std::filesystem;

std::mutex _io_mutex;
std::map<std::string, kiv_fs::Drive_Desc> registred_drivers;
std::vector<kiv_os::THandle> pipes;

STDHandle Register_STD() {
	const auto  in = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_Keyboard()));
	const auto out = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_VGA()));
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
	std::vector<kiv_fs::File_Desc> files;
	const auto res = kiv_fs::find_entire_dirs(drive, files, components);
	return res && fat_tool::is_attr(files.back().entire_dir.attributes, kiv_os::NFile_Attributes::Directory);
}

IOHandle* Open_File(std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes, kiv_os::NOS_Error& error) {
	std::lock_guard<std::mutex> locker(_io_mutex);

	std::vector<std::string> components;
	to_absolute_components(components, absolute_path);

	const auto drive_volume = components.front();
	components.erase(components.begin());

	const auto is_read_only = fat_tool::is_attr(static_cast<uint8_t>(attributes), kiv_os::NFile_Attributes::Read_Only);
	const auto drive = registred_drivers[drive_volume];

	if (fm == kiv_os::NOpen_File::fmOpen_Always) {
		if (drive_volume.compare("A:") == 0) {
			std::string sys;
			for (const auto& cmp : components) {
				sys += cmp;
			}
			return SYS_HANDLERS.at(sys);
		}
		else {
			std::vector<kiv_fs::File_Desc> files;
			if (!kiv_fs::find_entire_dirs(drive, files, components)) {
				error = kiv_os::NOS_Error::IO_Error;
				return nullptr;
			}
			const auto is_root = components.size() == 1;
			return new IOHandle_File(drive, files.back(), is_read_only, !is_root ? files.rbegin()[1].sectors : std::vector<uint16_t>(0));
		}
	}
	else {

		std::vector<kiv_fs::File_Desc> files;
		const auto exist = kiv_fs::find_entire_dirs(drive, files, components);

		kiv_fs::File_Desc new_file;

		if (exist) {
			new_file = files.back();
			files.pop_back();
			new_file.sectors.erase(new_file.sectors.begin() + 1, new_file.sectors.end());
		} else if (files.size() != components.size() - 1) {
			error = kiv_os::NOS_Error::IO_Error;
			return nullptr;
		}
		else {
			new_file.sectors = std::vector<uint16_t>();
			if (!kiv_fs::new_entire_dir(new_file.entire_dir, components.back(), static_cast<uint8_t>(attributes), error)) {
				return nullptr;
			}
		}

		const auto parrent_file = files.back();
		files.pop_back();

		if (!fat_tool::is_attr(parrent_file.entire_dir.attributes, kiv_os::NFile_Attributes::Directory)) {
			error = kiv_os::NOS_Error::IO_Error;
			return nullptr;
		}

		// find free sector for the new directory
		if (new_file.sectors.empty()) {
			std::vector<std::div_t> fat_offsets;
			if (!kiv_fs::find_free_sectors(fat_offsets, drive.id, START_OF_FAT, 1, drive.boot_block.bytes_per_sector)) {
				error = kiv_os::NOS_Error::IO_Error;
				return nullptr;
			}

			// save new directory to the FAT
			if (!kiv_fs::save_to_fat(drive.id, fat_offsets, drive.boot_block.bytes_per_sector, kiv_fs::offset(drive.boot_block), new_file.sectors, new_file.entire_dir.first_cluster)) {
				error = kiv_os::NOS_Error::IO_Error;
				return nullptr;
			}

			// uloz do nadrazene slozky entire_dir pro novou slozku
			if (!kiv_fs::save_to_dir(drive.id, parrent_file.sectors, drive.boot_block.bytes_per_sector, new_file.entire_dir, kiv_fs::Edit_Type::Add)) {
				error = kiv_os::NOS_Error::IO_Error;
				return nullptr;
			}

			if (!files.empty()) {
				auto grandparent_file = files.back();
				grandparent_file.entire_dir.size += sizeof kiv_fs::FATEntire_Directory;
				if (!kiv_fs::save_to_dir(drive.id, grandparent_file.sectors, drive.boot_block.bytes_per_sector, parrent_file.entire_dir, kiv_fs::Edit_Type::Edit)) {
					error = kiv_os::NOS_Error::IO_Error;
					return nullptr;
				}
			}
		}

		const auto is_newfile_dir = fat_tool::is_attr(new_file.entire_dir.attributes, kiv_os::NFile_Attributes::Directory);
		uint8_t permission = is_read_only ? Permission::Read : Permission::Read | Permission::Write;

		return is_newfile_dir ? new IOHandle() : new IOHandle_File(drive, new_file, permission, is_read_only ? std::vector<uint16_t>(0) : parrent_file.sectors);
	}
}

bool Remove_File(std::string absolute_path, kiv_os::NOS_Error& error) {
	std::lock_guard<std::mutex> locker(_io_mutex);

	std::vector<std::string> components;
	to_absolute_components(components, absolute_path);

	const auto drive = registred_drivers[components.front()];
	components.erase(components.begin());

	std::vector<kiv_fs::File_Desc> files;
	if (!kiv_fs::find_entire_dirs(drive, files, components)) {
		error = kiv_os::NOS_Error::File_Not_Found;
		return false;
	}

	const auto file_to_dell = files.back();
	files.pop_back();

	if (!fat_tool::is_attr(file_to_dell.entire_dir.attributes, kiv_os::NFile_Attributes::Directory)) {
		error = kiv_os::NOS_Error::Invalid_Argument;
		return false;
	}

	if (file_to_dell.entire_dir.size) {
		error = kiv_os::NOS_Error::Directory_Not_Empty;
		return false;
	}

	// free space in FAT
	kiv_fs::remove_sectors_in_fat(file_to_dell.entire_dir, drive.boot_block.bytes_per_sector, drive.id);

	// find parrent to remove Entry_dir
	auto parrent_file = files.back();
	files.pop_back();

	if (!kiv_fs::save_to_dir(drive.id, parrent_file.sectors, drive.boot_block.bytes_per_sector, file_to_dell.entire_dir, kiv_fs::Edit_Type::Del)) {
		error = kiv_os::NOS_Error::IO_Error;
		return false;
	}

	if (files.size() > 1) {
		// find grandparent
		const auto grandparrent_file = files.back();
		files.pop_back();

		parrent_file.entire_dir.size -= sizeof kiv_fs::FATEntire_Directory;
		if (!kiv_fs::save_to_dir(drive.id, grandparrent_file.sectors, drive.boot_block.bytes_per_sector, parrent_file.entire_dir, kiv_fs::Edit_Type::Edit)) {
			error = kiv_os::NOS_Error::IO_Error;
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
			kiv_os::NOS_Error error;
			const auto source = Open_File(path, fm, attributes, error);
			if (!source) {
				regs.flags.carry = 1;
				regs.rax.x = static_cast<decltype(regs.rax.x)>(error);
				break;
			}
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
			kiv_os::NOS_Error error;
			regs.flags.carry = !Remove_File(to_absolute_path(process->working_dir, path), error);
			regs.rax.x = static_cast<decltype(regs.rax.x)>(error);

		} break;

		case kiv_os::NOS_File_System::Close_Handle: {
			const auto handle = regs.rdx.x;
			const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(handle));
			if (!source) {
				regs.rax.x = static_cast<decltype(regs.rax.x)>(kiv_os::NOS_Error::Unknown_Error);
			}
			if (std::find(pipes.begin(), pipes.end(), handle) != pipes.end()) {
				source->close();
			}
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

		case kiv_os::NOS_File_System::Seek: {
			const auto source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
		} break;

		case kiv_os::NOS_File_System::Create_Pipe: {
			kiv_os::THandle* handle_ptr = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r);
			std::shared_ptr<Circular_buffer> pipe = std::make_shared<Circular_buffer>();
			auto pipe_to_write = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_Pipe(pipe, Permission::Write)));
			pipes.push_back(pipe_to_write);
			*(handle_ptr) = pipe_to_write;
			*(handle_ptr + 1) = Convert_Native_Handle(static_cast<HANDLE>(new IOHandle_Pipe(pipe, Permission::Read)));
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
