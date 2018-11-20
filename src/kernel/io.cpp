#include "io.h"
#include "kernel.h"
#include "handles.h"

#include "fat_tools.h"
#include "io_manager.h"
#include "common.h"

#include <filesystem>
#include <functional>

std::map<std::string, kiv_fs::Drive_Desc> registred_drivers;


STDHandle Register_STD() {
	auto console = new IOHandle_Console();
	const auto  in = Convert_Native_Handle(static_cast<HANDLE>(console));
	const auto out = Convert_Native_Handle(static_cast<HANDLE>(console));
	return { in, out };
}

IOHandle* Open_File(std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes) {

	std::vector<std::string> components;
	for (const auto& path : std::filesystem::path(absolute_path)) {
		components.push_back(path.generic_string());
	}
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

			const std::vector<size_t> sectors = kiv_fs::load_sectors(drive.boot_block, entry);

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
		kiv_fs::File_Desc file = { entry, std::vector<size_t>() };

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

		const auto sectors = kiv_fs::load_sectors(drive.boot_block, parrent_entry);

		kiv_fs::File_Desc parrent_file = { parrent_entry, sectors };

		if (!kiv_fs::create_dir(drive, parrent_file, file)) {
			// TODO error -> cannot create new dir
		}

		return new IOHandle();
	}
}

void Handle_IO(kiv_hal::TRegisters &regs) {

	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {

		case kiv_os::NOS_File_System::Open_File: {
			IOHandle* source = Open_File(reinterpret_cast<char*>(regs.rdx.r), static_cast<kiv_os::NOpen_File>(regs.rcx.l), static_cast<kiv_os::NFile_Attributes>(regs.rdi.i));
			regs.rax.x = Convert_Native_Handle(static_cast<HANDLE>(source));
		} break;

		case kiv_os::NOS_File_System::Read_File: {
			IOHandle* source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
			regs.rax.r = source->read(reinterpret_cast<char*>(regs.rdi.r), regs.rcx.r);
		} break;

		case kiv_os::NOS_File_System::Write_File: {
			IOHandle* source = static_cast<IOHandle*>(Resolve_kiv_os_Handle(regs.rdx.x));
			regs.rax.r = source->write(reinterpret_cast<char*>(regs.rdi.r), regs.rcx.r);
			regs.flags.carry |= (regs.rax.r == 0 ? 1 : 0);	//jestli jsme nezapsali zadny znak, tak jiste doslo k nejake chybe
		} break;

		case kiv_os::NOS_File_System::Close_Handle: {
			regs.rax.x = Remove_Handle(regs.rdx.x);
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
			const auto buffer = reinterpret_cast<char*>(regs.rdx.r);
			auto str_path = std::string(buffer, strlen(buffer));
			const auto process = process_manager->getRunningProcess();

			if (std::filesystem::u8path(str_path).is_relative()) {
				auto path = std::filesystem::u8path(process->working_dir);
				path /= str_path;
				str_path = path.lexically_normal().u8string();
			}

			const auto handle = Open_File(str_path, kiv_os::NOpen_File::fmOpen_Always, kiv_os::NFile_Attributes::Directory);
			if (handle) {
				process->working_dir = str_path;
				delete handle;
			}
			else {
				regs.flags.carry = 1;
				regs.rax.x = static_cast<decltype(regs.rax.x)>(kiv_os::NOS_Error::File_Not_Found);
			}

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
