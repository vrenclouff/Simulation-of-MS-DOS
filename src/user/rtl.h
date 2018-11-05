#pragma once

#include "..\api\api.h"
#include <atomic>
#include <string>

namespace kiv_os_rtl {

	extern std::atomic<kiv_os::NOS_Error> Last_Error;

	//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
	//vraci true, kdyz vse OK
	//vraci true, kdyz vse OK
	bool Read_File(const kiv_os::THandle file_handle, char* const buffer, const size_t buffer_size, size_t &read);

	//zapise do souboru identifikovaneho deskriptor data z buffer o velikosti buffer_size a vrati pocet zapsanych dat ve written
	//vraci true, kdyz vse OK
	//vraci true, kdyz vse OK
	bool Write_File(const kiv_os::THandle file_handle, const char *buffer, const size_t buffer_size, size_t &written);

	kiv_os::THandle Clone(char* args);

	bool Wait_For(kiv_os::THandle handlers[]);

	bool Exit(bool exitcode);

	bool Read_Exit_Code(kiv_os::THandle handle);

	bool Shutdown();

}
