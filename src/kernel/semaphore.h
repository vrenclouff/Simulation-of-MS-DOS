#pragma once

#include <mutex>
#include <condition_variable>

class Semaphore {

public:

	Semaphore(int count_ = 0) : count(count_) {}

	void p();

	void v();

private:

	std::mutex mtx;

	std::condition_variable cv;

	int count;

};
