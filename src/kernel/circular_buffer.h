#pragma once

#include <mutex>
#include <condition_variable>

#define BUFFER_SIZE 1024

class Circular_buffer {

private:
	std::mutex _mutex;
	std::condition_variable _cond;

	char buffer[BUFFER_SIZE];
	size_t start, end = 0;
	bool full = false;

	bool empty() const;

public:
	size_t write(const char item);
	char read();
	size_t size() const;
};
