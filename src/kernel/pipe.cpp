#include "pipe.h"

bool Pipe::isEmpty() {
	return (!full && (start == end));
}

size_t Pipe::getSize() {
	size_t size = 0;

	if (!full)
	{
		if (start >= end)
		{
			size = start - end;
		}
		else
		{
			size = (start - end) + max;
		}
	}

	return size;
}

bool Pipe::hasEnoughSpace(size_t buffer_size) {

	size_t freeSpace = max - getSize();

	return (freeSpace >= buffer_size);
}

size_t Pipe::write(char to_write) {

	writeSem->p(); // cekej, pokud neni misto pro zapis

	ring[start] = to_write;

	if (full)
	{
		end = (end + 1) % max;
	}

	start = (start + 1) % max;

	full = (start == end);

	readSem->v(); // nyni lze dalsi precist

	return 0;
}

char Pipe::read() {

	readSem->p(); // cekej, dokud neni co cist

	if (isEmpty())
	{
		return 0; //TODO
	}

	auto val = ring[end];
	full = false;
	end = (end + 1) % max;

	writeSem->v(); // nyni lze dalsi zapsat

	return val;
}
