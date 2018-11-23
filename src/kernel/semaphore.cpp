#include "semaphore.h"

void Semaphore::p()
{
	std::unique_lock<std::mutex> lock(mtx);

	while (count == 0) {
		cv.wait(lock);
	}
	count--;
}

void Semaphore::v()
{
	std::unique_lock<std::mutex> lock(mtx);
	count++;
	cv.notify_one();
}
