#include "io.h"
#include "kernel.h"
#include "handles.h"

#include "fat_file.h"
#include "fat_tools.h"
#include "iohandle.h"

#include <filesystem>
#include <functional>

//kiv_os::THandle working_dir;
std::string working_dir;

STDHandle Register_STD() {
	IOHandle* console = IOHandle::console();
	const auto  in = Convert_Native_Handle(static_cast<HANDLE>(console));
	const auto out = Convert_Native_Handle(static_cast<HANDLE>(console));
	return { in, out };
}

kiv_os::THandle Open_File(std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes) {

	// TODO 
	/*
		tady urcit co chceme otevrit

		fat_file		-> soubor ktery by mel pracovat s otevrenym souborem read & write
		absolute_path	-> obsahuje cestu k souboru/slozce nebo SYS
		fm				-> urcuje pokud soubor existuje a bude se jen otevirat (hledat na disku) nebo ho mame vytvorit
		attributes		-> pokud soubor existuje zajima nas jen read_only, pokud neexistuje vytvor soubor se vsema atributama
						   kontrola, zda oteviram stejny typ souboru -> attribute urcuji slozku, tak mam otevrit slozku -> pripadne error
	
		disky A: a B: rezerovany pro system

	*/

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
			return Convert_Native_Handle(static_cast<HANDLE>(SYS_HANDLERS.at(sys)));
		}
		else {
			// existuje file / dir -> najit


		}
	}
	else {
		// neexistuje -> vytvorit ve FAT s velikosti sektoru a prirazenim 1 sektorem

		// najit entire_dir pro novy soubor


		// file / dir
	}

	//return io::open(drive_volume, path_components);
	return Convert_Native_Handle(NULL);
}

void Handle_IO(kiv_hal::TRegisters &regs) {

	switch (static_cast<kiv_os::NOS_File_System>(regs.rax.l)) {

		case kiv_os::NOS_File_System::Open_File: {
			regs.rax.x = Open_File(reinterpret_cast<char*>(regs.rdx.r), static_cast<kiv_os::NOpen_File>(regs.rcx.l), static_cast<kiv_os::NFile_Attributes>(regs.rdi.i));
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
			// TODO predelat na THandle
			const size_t size = buffer_size <= working_dir.size() ? buffer_size : working_dir.size();
			std::copy(&working_dir[0], &working_dir[0] + size, path_buffer);
			regs.rax.r = size;
		} break;

		case kiv_os::NOS_File_System::Set_Working_Dir: {
			const auto path_buffer = reinterpret_cast<char*>(regs.rdx.r);
			const auto buffer_size = regs.rcx.r;
			// TODO predelat na THandle
			working_dir = std::string(path_buffer, buffer_size);
		} break;

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
	}
}
