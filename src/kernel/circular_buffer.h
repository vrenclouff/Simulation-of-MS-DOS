#pragma once

#include "semaphore.h"

#define BUFFER_SIZE 2

class Circular_buffer {

private:
	std::mutex _mutex;
	std::condition_variable _cond;

	// Semaphore* write_Sem = new Semaphore(BUFFER_SIZE);
	// Semaphore* read_Sem = new Semaphore(0);

	char buffer[BUFFER_SIZE];
	size_t start, end = 0;
	bool full = false;

	bool empty() const;

public:
	//~Pipe() {
	//	delete write_Sem;
	//	delete read_Sem;
	//}

	size_t write(const char item);
	char read();

	size_t size() const;
};
