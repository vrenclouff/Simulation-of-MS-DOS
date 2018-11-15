#include "test.h"
#include "rtl.h"

#include <cstdlib>
#include <ctime>
#include <random>
# include <windows.h>
#include <vector>

// ----- HELPER FUNCTIONS -----
template<typename T>
void assertEqual(T first, T second, uint16_t std_out)
{
	if (first == second)
	{
		print("PASS\n", std_out);
		return;
	}
	print("FAIL\n", std_out);
}

template<typename T>
void assertNotEqual(T first, T second, uint16_t std_out)
{
	if (first != second)
	{
		print("PASS\n", std_out);
		return;
	}
	print("FAIL\n", std_out);
}

// ----- SYSCALLS -----

kiv_os::THandle createProcess(char* name, char* args, uint16_t stdin_handle, uint16_t stdout_handle, kiv_os::NOS_Error &error, kiv_hal::TFlags &flags)
{
	kiv_hal::TRegisters contx;
	contx.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	contx.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Clone);
	contx.rcx.r = static_cast<uint64_t>(kiv_os::NClone::Create_Process);
	contx.rdx.r = reinterpret_cast<decltype(contx.rdx.r)>(name);
	contx.rdi.r = reinterpret_cast<decltype(contx.rdi.r)>(args);
	contx.rbx.e = (stdin_handle << 16) | stdout_handle;
	kiv_os::Sys_Call(contx);
	error = kiv_os_rtl::Last_Error;
	flags = contx.flags;
	return static_cast<kiv_os::THandle>(contx.rax.x);
}

kiv_os::THandle createThread(void* func, void* data, uint16_t stdin_handle, uint16_t stdout_handle, kiv_os::NOS_Error &error, kiv_hal::TFlags &flags)
{
	kiv_hal::TRegisters contx;
	contx.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	contx.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Clone);
	contx.rcx.r = static_cast<uint64_t>(kiv_os::NClone::Create_Thread);
	contx.rdx.r = reinterpret_cast<decltype(contx.rdx.r)>(func);
	contx.rdi.r = reinterpret_cast<decltype(contx.rdi.r)>(data);
	contx.rbx.e = (stdin_handle << 16) | stdout_handle;
	kiv_os::Sys_Call(contx);
	error = kiv_os_rtl::Last_Error;
	flags = contx.flags;
	return static_cast<kiv_os::THandle>(contx.rax.x);
}

kiv_os::THandle waitFor(kiv_os::THandle* handles, int size, kiv_os::NOS_Error &error, kiv_hal::TFlags &flags)
{
	kiv_hal::TRegisters contx;
	contx.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	contx.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Wait_For);
	contx.rdx.r = reinterpret_cast<uint64_t>(handles);
	contx.rcx.r = size;
	kiv_os::Sys_Call(contx);
	error = kiv_os_rtl::Last_Error;
	flags = contx.flags;
	return static_cast<kiv_os::THandle>(contx.rax.x);
}

void exitCall(int return_value)
{
	kiv_hal::TRegisters contx;
	contx.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	contx.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Exit);
	contx.rcx.x = return_value;
	kiv_os::Sys_Call(contx);
}

uint16_t readExitCode(kiv_os::THandle handle, kiv_os::NOS_Error &error, kiv_hal::TFlags &flags)
{
	kiv_hal::TRegisters contx;
	contx.rax.h = static_cast<uint8_t>(kiv_os::NOS_Service_Major::Process);
	contx.rax.l = static_cast<uint8_t>(kiv_os::NOS_Process::Read_Exit_Code);
	contx.rdx.x = handle;
	kiv_os::Sys_Call(contx);
	uint16_t exit_code = contx.rcx.x;
	error = kiv_os_rtl::Last_Error;
	flags = contx.flags;
	return exit_code;
}

void print(const char* msg, uint16_t std_out)
{
	size_t counter;
	kiv_os_rtl::Write_File(std_out, msg, strlen(msg), counter);
}

// ----- USER FUNCTIONS -----

size_t __stdcall test(const kiv_hal::TRegisters &regs)
{
	stdOut(regs);

	serialProcesses(regs);

	paralelProcessesMultiWaitFor(regs);

	paralelProcessesSerialWaitFor(regs);

	readExitCodeTest(regs);

	waitForTest(regs);

	createProcessTest(regs);

	createThreadTest(regs);

	// call exit
	exitCall(0);
	return 0;
}

size_t __stdcall test_delay(const kiv_hal::TRegisters &regs) {
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	// Random delay
	std::mt19937 rng;
	rng.seed(std::random_device()());
	std::uniform_int_distribution<std::mt19937::result_type> rand_wait(1, 500);

	int delay = rand_wait(rng);
	std::string msg = "Sleeping for " + std::to_string(delay) + "\n";
	print(msg.c_str(), std_out);
	Sleep(delay);
	print("Sleeping ended\n", std_out);

	// call exit
	exitCall(0);
	return 0;
}

size_t __stdcall test_exit0(const kiv_hal::TRegisters &regs)
{
	// call exit
	exitCall(0);
	return 0;
}

size_t __stdcall test_exit1(const kiv_hal::TRegisters &regs)
{
	// call exit
	exitCall(1);
	return 0;
}

size_t __stdcall test_long_delay(const kiv_hal::TRegisters &regs)
{
	Sleep(2000);
	// call exit
	exitCall(0);
	return 0;
}

void test_thread(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	bool* data = reinterpret_cast<bool*>(regs.rdi.r);
	*data = true;
	print("PRINT MESSAGE FROM THREAD\n", std_out);
	// call exit
	exitCall(0);
	return;
}

size_t __stdcall test_open_file(const kiv_hal::TRegisters & regs) {

	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	//std::string file_name("lorem2.txt");
	std::string file_name("C://home/test.txt");

	kiv_os::THandle file_handle;
	kiv_os_rtl::Open_File(file_name.c_str(), file_name.size(), file_handle, false, std::iostream::ios_base::in);

	char buffer[5];
	size_t counter;
	kiv_os_rtl::Read_File(file_handle, buffer, sizeof(buffer), counter);

	auto str = std::string(buffer, counter);

	const auto close_handle = kiv_os_rtl::Close_Handle(file_handle);

	return 0;
}


// -------------TESTS---------------

void stdOut(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);

	// STDOUT TEST
	print("Test output\n", std_out);
}

void serialProcesses(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	print("\n --------TESTING SERIAL CREATEPROCESS AND WAITFOR-------- \n\n", std_out);

	for (int i = 0; i < 10; i++)
	{
		kiv_os::NOS_Error error;
		kiv_hal::TFlags flags;
		// Create process syscall
		kiv_os::THandle child_handle = createProcess("test_delay", "", std_in, std_out, error, flags);

		// Call waitfor
		waitFor(&child_handle, 1, error, flags);
		readExitCode(child_handle, error, flags);
	}
}

void paralelProcessesMultiWaitFor(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	print("\n --------TESTING PARALEL CREATEPROCESS AND MULTIPLE WAITFOR-------- \n\n", std_out);

	std::vector<kiv_os::THandle> handles;
	for (int i = 0; i < 10; i++)
	{
		// Create process syscall
		kiv_os::NOS_Error error;
		kiv_hal::TFlags flags;
		handles.push_back(createProcess("test_delay", "", std_in, std_out, error, flags));
	}

	// Call waitfor
	kiv_os::NOS_Error error;
	kiv_hal::TFlags flags;
	kiv_os::THandle first_handle = waitFor(&handles[0], 10, error, flags);
	readExitCode(first_handle, error, flags);
	std::string msg = "First finished handle is " + std::to_string(first_handle) + "\n";
	print(msg.c_str(), std_out);

	for (int i = 0; i < 10; i++)
	{
		if (handles[i] == first_handle)
		{
			continue;
		}
		waitFor(&handles[i], 1, error, flags);
		readExitCode(handles[i], error, flags);
	}
}

void paralelProcessesSerialWaitFor(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	print("\n --------TESTING PARALEL CREATEPROCESS AND SERIAL WAITFOR-------- \n\n", std_out);

	std::vector<kiv_os::THandle> handles;
	for (int i = 0; i < 10; i++)
	{
		// Create process syscall
		kiv_os::NOS_Error error;
		kiv_hal::TFlags flags;
		handles.push_back(createProcess("test_delay", "", std_in, std_out, error, flags));
	}

	for (int i = 0; i < 10; i++)
	{
		// Call waitfor
		kiv_os::NOS_Error error;
		kiv_hal::TFlags flags;
		std::string msg = "Waiting for handle " + std::to_string(handles[i]) + "\n";
		print(msg.c_str(), std_out);
		kiv_os::THandle handle_completed = waitFor(&handles[i], 1, error, flags);
		readExitCode(handle_completed, error, flags);
		std::string msg2 = "Handle " + std::to_string(handle_completed) + "ended\n";
		print(msg2.c_str(), std_out);
	}
}

void readExitCodeTest(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	print("\n --------TESTING READ EXIT CODE-------- \n\n", std_out);

	// EXIT CODE 0
	kiv_os::NOS_Error error;
	kiv_hal::TFlags flags;
	print("Testing exit code 0\n", std_out);
	kiv_os::THandle handle = createProcess("test_exit0", "", std_in, std_out, error, flags);
	waitFor(&handle, 1, error, flags);
	uint16_t exitCode = readExitCode(handle, error, flags);
	assertEqual<uint16_t>(exitCode, 0, std_out);
	assertEqual<kiv_os::NOS_Error>(error, kiv_os::NOS_Error::Success, std_out);
	assertEqual<std::uint8_t>(flags.carry, 0, std_out);

	// EXIT CODE 1
	print("Testing exit code 1\n", std_out);
	handle = createProcess("test_exit1", "", std_in, std_out, error, flags);
	waitFor(&handle, 1, error, flags);
	exitCode = readExitCode(handle, error, flags);
	assertEqual<uint16_t>(exitCode, 1, std_out);
	assertEqual<kiv_os::NOS_Error>(error, kiv_os::NOS_Error::Success, std_out);
	assertEqual<std::uint8_t>(flags.carry, 0, std_out);

	// READ EXIT CODE BEFORE END
	print("Testing exit call before process end\n", std_out);
	handle = createProcess("test_long_delay", "", std_in, std_out, error, flags);
	exitCode = readExitCode(handle, error, flags);
	assertNotEqual<kiv_os::NOS_Error>(error, kiv_os::NOS_Error::Success, std_out);
	assertNotEqual<std::uint8_t>(flags.carry, 0, std_out);
	waitFor(&handle, 1, error, flags);
	readExitCode(handle, error, flags);
}

void waitForTest(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	print("\n --------TESTING WAITFOR-------- \n\n", std_out);

	kiv_os::THandle handle;
	kiv_os::NOS_Error error;
	kiv_hal::TFlags flags;

	// Valid handle
	print("Testing valid handle\n", std_out);
	handle = createProcess("test_exit1", "", std_in, std_out, error, flags);
	waitFor(&handle, 1, error, flags);
	assertEqual<std::uint8_t>(flags.carry, 0, std_out);
	readExitCode(handle, error, flags);

	// Invalid handle
	print("Testing invalid handle\n", std_out);
	handle = createProcess("test_exit1", "", std_in, std_out, error, flags);
	kiv_os::THandle invalid = kiv_os::Invalid_Handle;
	waitFor(&invalid, 1, error, flags);
	assertNotEqual<std::uint8_t>(flags.carry, 0, std_out);
	assertNotEqual<kiv_os::NOS_Error>(error, kiv_os::NOS_Error::Success, std_out);

	waitFor(&handle, 1, error, flags);
	readExitCode(handle, error, flags);
}

void createProcessTest(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	print("\n --------TESTING CREATEPROCESS-------- \n\n", std_out);

	kiv_os::THandle handle;
	kiv_os::NOS_Error error;
	kiv_hal::TFlags flags;

	// Valid function name
	print("Testing valid function name\n", std_out);
	handle = createProcess("test_exit1", "", std_in, std_out, error, flags);
	assertEqual<std::uint8_t>(flags.carry, 0, std_out);
	waitFor(&handle, 1, error, flags);
	readExitCode(handle, error, flags);

	// Invalid function name
	print("Testing invalid function name\n", std_out);
	handle = createProcess("nonexistentfunction", "", std_in, std_out, error, flags);
	assertNotEqual<std::uint8_t>(flags.carry, 0, std_out);
	assertNotEqual<kiv_os::NOS_Error>(error, kiv_os::NOS_Error::Success, std_out);
}

void createThreadTest(const kiv_hal::TRegisters &regs)
{
	const kiv_os::THandle std_out = static_cast<kiv_os::THandle>(regs.rbx.x);
	const kiv_os::THandle std_in = static_cast<kiv_os::THandle>(regs.rax.x);
	print("\n --------TESTING CREATETHREAD-------- \n\n", std_out);

	kiv_os::THandle handle;
	kiv_os::NOS_Error error;
	kiv_hal::TFlags flags;

	// Valid function pointer
	print("Testing shared memory change\n", std_out);
	bool data = false;
	handle = createThread(&test_thread, &data, std_in, std_out, error, flags);
	assertEqual<std::uint8_t>(flags.carry, 0, std_out);
	waitFor(&handle, 1, error, flags);
	assertEqual<bool>(data, true, std_out);
	readExitCode(handle, error, flags);
}
