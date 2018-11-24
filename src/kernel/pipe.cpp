#include "pipe.h"

bool Pipe::is_Empty() {
	return (!full && (start == end));
}

size_t Pipe::get_Size() {
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

size_t Pipe::write(char to_write) {

	write_Sem->p(); // cekej, pokud neni misto pro zapis

	ring[start] = to_write;

	if (full)
	{
		end = (end + 1) % max;
	}

	start = (start + 1) % max;

	full = (start == end);

	read_Sem->v(); // nyni lze dalsi precist

	return 0;
}

char Pipe::read() {

	read_Sem->p(); // cekej, dokud neni co cist

	if (is_Empty())
	{
		return 0; //TODO
	}

	auto val = ring[end];
	full = false;
	end = (end + 1) % max;

	write_Sem->v(); // nyni lze dalsi zapsat

	return val;
}
