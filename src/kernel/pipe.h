#pragma once

#include "semaphore.h"

class Pipe {

public:

	Pipe::Pipe() {
		this->start = 0;
		this->end = 0;
		this->full = false;
		this->writeSem = new Semaphore(this->max);
		this->readSem = new Semaphore(0);
	}

	size_t write(char to_write);
	char read();
	bool hasEnoughSpace(size_t buffer_size);
	size_t getSize();
	bool isEmpty();

private:

	size_t max = 256;
	char ring[256];
	size_t start;
	size_t end;
	bool full;
	Semaphore* writeSem;
	Semaphore* readSem;

};
