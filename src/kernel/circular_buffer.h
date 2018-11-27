#pragma once

#include <mutex>
#include <condition_variable>

#define BUFFER_SIZE 1024

class Circular_buffer {

private:
	std::mutex _mutex;
	std::condition_variable _cond;

	char buffer[BUFFER_SIZE];
	size_t start = 0, end = 0;
	bool full = false;

	bool empty() const;

public:
	void close();
	bool is_EOF = false;
	size_t write(const char item);
	int read();
	size_t size() const;
};
