#pragma once

#include <map>
#include <functional>

enum class SYS_Type : uint8_t {
	PROCFS = 1,
};

namespace iohandle {
	namespace console {
		size_t read(char* buffer, size_t buffer_size);
		size_t write(char* buffer, size_t buffer_size);
	}

	namespace file {
		size_t read(char* buffer, size_t buffer_size);
		size_t write(char* buffer, size_t buffer_size);
	}

	namespace sys {
		size_t procfs(char * buffer, size_t buffer_size);
	}
}

class IOHandle {
private:
	std::function<size_t(char*, size_t)> write_fnc;
	std::function<size_t(char*, size_t)> read_fnc;

public:
	static IOHandle* console() { 
		auto instance = new IOHandle(); 
		instance->write_fnc = std::bind(&iohandle::console::write, std::placeholders::_1, std::placeholders::_2);
		instance->read_fnc = std::bind(&iohandle::console::read, std::placeholders::_1, std::placeholders::_2);
		return instance;
	}
	static IOHandle* file()	{
		auto instance = new IOHandle();
		instance->write_fnc = std::bind(iohandle::file::write, std::placeholders::_1, std::placeholders::_2);
		instance->read_fnc = std::bind(iohandle::file::read, std::placeholders::_1, std::placeholders::_2);
		return instance;
	}
	static IOHandle* sys(const SYS_Type sys_type) { 
		auto instance = new IOHandle();
		instance->read_fnc = [=](char* buffer, size_t buffer_size) {
			switch (sys_type) {
				case SYS_Type::PROCFS: return iohandle::sys::procfs(buffer, buffer_size);
				default: return size_t();
			}
		};
		return instance;
	}

	size_t write(char* buffer, size_t buffer_size) {
		return write_fnc(buffer, buffer_size);
	}
	
	size_t read(char* buffer, size_t buffer_size) {
		return read_fnc(buffer, buffer_size);
	}
};

static const std::map<std::string, IOHandle*> SYS_HANDLERS = {
	{"/procfs", IOHandle::sys(SYS_Type::PROCFS)}
};