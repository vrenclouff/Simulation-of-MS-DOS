#include "io.h"
#include "kernel.h"
#include "handles.h"

#include "fat_tools.h"
#include "io_manager.h"
#include "common.h"

#include <filesystem>
#include <functional>


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
			kiv_fs::Drive_Desc drive;
			kiv_fs::File_Desc file;
			io::open(drive, file, drive_volume, components);
			return new IOHandle_File(drive, file);
		}
	}
	else {
		// TODO neexistuje -> file / dir

		// vytvorit ve FAT s velikosti sektoru a prirazenim 1 sektorem
		// najit entire_dir pro novy soubor
		return NULL;
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
			const auto path_buffer = reinterpret_cast<char*>(regs.rdx.r);
			const auto process = process_manager->getRunningProcess();
			process->working_dir = std::string(path_buffer, strlen(path_buffer));
		} break;
	}
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
