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

	bool Get_Working_Dir(const char *buffer, const size_t buffer_size, size_t &read);

	bool Set_Working_Dir(const char *dir);

	bool Open_File(const char *buffer, const size_t buffer_size, kiv_os::THandle &file_handle, const bool exist, const kiv_os::NFile_Attributes attributes);

	bool Delete_File(const char* filename);

	bool Close_Handle(const kiv_os::THandle file_handle);

	kiv_os::THandle* Create_Pipe();

	kiv_os::THandle Clone(const char* function, const char* arguments, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle);

	bool Wait_For(kiv_os::THandle handlers[]);

	bool Exit(uint16_t exitcode);

	std::uint8_t Read_Exit_Code(kiv_os::THandle handle);

	bool Shutdown();

	bool Register_Signal_Handler();

	kiv_os::THandle Create_Thread(void* function, void* data, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle);

}
