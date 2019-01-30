#pragma once

#include "..\api\api.h"

// @author: Petr Volf & Lukas Cerny

extern "C" size_t __stdcall test(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall test_delay(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall test_exit0(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall test_exit1(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall test_long_delay(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall test_write_to_file(const kiv_hal::TRegisters &regs);
extern "C" size_t __stdcall test_pipe(const kiv_hal::TRegisters &regs);

void test_thread(const kiv_hal::TRegisters &regs);

// Syscalls
kiv_os::THandle createProcess(char* name, char* args, uint16_t stdin_handle, uint16_t stdout_handle, kiv_os::NOS_Error &error, kiv_hal::TFlags &flags);
kiv_os::THandle createThread(void* func, void* data, uint16_t stdin_handle, uint16_t stdout_handle, kiv_os::NOS_Error &error, kiv_hal::TFlags &flags);
kiv_os::THandle waitFor(kiv_os::THandle* handles, int size, kiv_os::NOS_Error &error, kiv_hal::TFlags &flags);
void exitCall(int return_value);
uint16_t readExitCode(kiv_os::THandle handle, kiv_os::NOS_Error &error, kiv_hal::TFlags &flags);
void print(const char* msg, uint16_t std_out);

// tests
void stdOut(const kiv_hal::TRegisters &regs);
void serialProcesses(const kiv_hal::TRegisters &regs);
void paralelProcessesMultiWaitFor(const kiv_hal::TRegisters &regs);
void paralelProcessesSerialWaitFor(const kiv_hal::TRegisters &regs);
void readExitCodeTest(const kiv_hal::TRegisters &regs);
void waitForTest(const kiv_hal::TRegisters &regs);
void createProcessTest(const kiv_hal::TRegisters &regs);
void createThreadTest(const kiv_hal::TRegisters &regs);

// helpers
template<typename T>
void assertEqual(T first, T second, uint16_t std_out);

template<typename T>
void assertNotEqual(T first, T second, uint16_t std_out);
