//#define CRT
//#define VLD

#include <Windows.h>
#include <iostream>

#include "../api/hal.h"
#include "idt.h"
#include "keyboard.h"

#ifdef VLD
#include "C:\\Program Files (x86)\\Visual Leak Detector\\include\\vld.h"
#pragma comment(lib, "C:\\Program Files (x86)\\Visual Leak Detector\\lib\\Win64\\vld.lib")
extern "C" char __ImageBase;
#endif

#ifdef CRT
#define MYDEBUG_NEW   new( _NORMAL_BLOCK, __FILE__, __LINE__)
#define new MYDEBUG_NEW
#endif


bool Setup_HW() {
	if (!Init_Keyboard()) {
		std::wcout << L"Nepodarilo se vypnout konzolove echo, mohly by byt chyby na vystupu => koncime." << std::endl;
		return false;
	}


	//pripraveme tabulku vektoru preruseni
	if (!Init_IDT()) {
		std::wcout << L"Nepodarilo se spravne inicializovat IDT!" << std::endl;
		return false;
	}

	//disky tady inicializovat nebudeme, ty at si klidne selzou treba behem chodu systemu

	return true;
}

int __cdecl main() {

#ifdef VLD
	VLDEnable();
#endif
#ifdef CRT
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);

	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

	if (!Setup_HW()) return 1;

	//HW je nastaven, zavedeme simulovany operacni system
	HMODULE kernel = LoadLibraryW(L"kernel.dll");
	if (!kernel) {
		std::wcout << L"Nelze nacist kernel.dll!" << std::endl;
		return 1;
	}

	//v tuto chvili DLLMain v kernel.dll mela nahrat na NInterrupt::Bootstrap_Loader adresu pro inicializaci jadra
	//takze ji spustime
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Bootstrap_Loader, kiv_hal::TRegisters{ 0 });	

	
	//a az simulovany OS skonci, uvolnime zdroje z pameti
	FreeLibrary(kernel);
	TlsFree(kiv_hal::Expected_Tls_IDT_Index);

#ifdef VLD
	VLDReportLeaks();
#endif
#ifdef CRT
	_CrtDumpMemoryLeaks();
	//_CrtMemDumpAllObjectsSince(NULL);
#endif

	return 0;
}