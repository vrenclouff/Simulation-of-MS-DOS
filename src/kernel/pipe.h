#pragma once

#include "semaphore.h"

class Pipe {

public:

	Pipe::Pipe() {
		this->start = 0;
		this->end = 0;
		this->full = false;
		this->write_Sem = new Semaphore((int) this->max);
		this->read_Sem = new Semaphore(0);
	}

	size_t write(char to_write);
	char read();
	size_t get_Size();

private:

	size_t max = 256;
	char ring[256];
	size_t start;
	size_t end;
	bool full;
	Semaphore* write_Sem;
	Semaphore* read_Sem;
	bool is_Empty();

};
