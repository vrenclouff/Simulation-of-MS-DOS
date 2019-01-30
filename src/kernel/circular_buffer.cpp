#include "circular_buffer.h"

// @author: Lukas Cerny

size_t Circular_buffer::size() const {
	return !full ? (start >= end ? start - end : ((start - end) + BUFFER_SIZE)) : BUFFER_SIZE;
}

bool Circular_buffer::empty() const {
	return size() == 0;
}

size_t Circular_buffer::write(const char item) {
	std::unique_lock<std::mutex> locker(_mutex);
	_cond.wait(locker, [&](){return size() < BUFFER_SIZE; });

	buffer[start] = item;

	if (full) {
		end = (end + 1) % BUFFER_SIZE;
	}

	start = (start + 1) % BUFFER_SIZE;
	full = (start == end);

	locker.unlock();
	_cond.notify_all();
		
	return 0;
}

void Circular_buffer::close() {
	std::lock_guard<std::mutex> locker(_mutex);
	is_EOF = true;
	_cond.notify_all();
}

int Circular_buffer::read() {
	std::unique_lock<std::mutex> locker(_mutex);
	_cond.wait(locker, [&]() {return size() > 0 || is_EOF; });

	if (is_EOF && empty()) return -1;

	auto val = buffer[end];
	full = false;
	end = (end + 1) % BUFFER_SIZE;

	locker.unlock();
	_cond.notify_all();

	return val;
}
