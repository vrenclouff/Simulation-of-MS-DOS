#include "iohandle.h"

#include "../api/hal.h"

size_t iohandle::console::write(char * buffer, size_t buffer_size) {
	kiv_hal::TRegisters regs;
	regs.rax.h = static_cast<decltype(regs.rax.h)>(kiv_hal::NVGA_BIOS::Write_String);
	regs.rdx.r = reinterpret_cast<decltype(regs.rdx.r)>(buffer);
	regs.rcx.r = buffer_size;
	kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, regs);
	return regs.rcx.r;
}

size_t  iohandle::console::read(char * buffer, size_t buffer_size) {
	kiv_hal::TRegisters registers;

	size_t pos = 0;
	while (pos < buffer_size) {
		//read char
		registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NKeyboard::Read_Char);
		kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::Keyboard, registers);

		char ch = registers.rax.l;

		//osetrime zname kody
		switch (static_cast<kiv_hal::NControl_Codes>(ch)) {
		case kiv_hal::NControl_Codes::BS: {
			//mazeme znak z bufferu
			if (pos > 0) pos--;

			registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_Control_Char);
			registers.rdx.l = ch;
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
		}
										  break;

		case kiv_hal::NControl_Codes::LF:  break;	//jenom pohltime, ale necteme
		case kiv_hal::NControl_Codes::NUL:			//chyba cteni?
		case kiv_hal::NControl_Codes::CR:  return pos;	//docetli jsme az po Enter

		default: buffer[pos] = ch;
			pos++;
			registers.rax.h = static_cast<decltype(registers.rax.l)>(kiv_hal::NVGA_BIOS::Write_String);
			registers.rdx.r = reinterpret_cast<decltype(registers.rdx.r)>(&ch);
			registers.rcx.r = 1;
			kiv_hal::Call_Interrupt_Handler(kiv_hal::NInterrupt::VGA_BIOS, registers);
			break;
		}
	}

	return pos;
}

size_t iohandle::file::write(char * buffer, size_t buffer_size) {
	return size_t();
}

size_t iohandle::file::read(char * buffer, size_t buffer_size) {
	return size_t();
}

size_t iohandle::sys::procfs(char * buffer, size_t buffer_size) {

	const auto head = "PID \t PPID \t STIME \t COMMAND \n";
	std::copy(head, head + strlen(head), buffer);

	return strlen(head);
}
