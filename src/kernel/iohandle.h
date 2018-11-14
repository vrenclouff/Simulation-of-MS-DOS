#pragma once

#include <map>
#include <functional>

#include "fat.h"

namespace param = std::placeholders;

enum class SYS_Type : uint8_t {
	PROCFS = 1,
};

namespace iohandle {
	namespace console {
		size_t read(char* buffer, size_t buffer_size);
		size_t write(char* buffer, size_t buffer_size);
	}

	namespace file {
		size_t read(const kiv_fs::Drive_Desc& drive, const kiv_fs::FATEntire_Directory& entire_dir, char* buffer, const size_t buffer_size, size_t& seek);
		size_t write(char* buffer, size_t buffer_size);
	}

	namespace sys {
		size_t procfs(char * buffer, size_t buffer_size);
	}
}

class IOHandle {
private:
	std::function<size_t(char*, size_t)> _write;
	std::function<size_t(char*, const size_t)> _read;

	size_t seek = 0;

public:
	static IOHandle* console() { 
		auto instance = new IOHandle(); 
		instance->_write = std::bind(&iohandle::console::write, param::_1, param::_2);
		instance->_read  = std::bind(&iohandle::console::read,  param::_1, param::_2);
		return instance;
	}
	static IOHandle* file(const kiv_fs::Drive_Desc& drive, const kiv_fs::FATEntire_Directory& entire_dir) {
		auto instance = new IOHandle();
		size_t* seek = &instance->seek;
		instance->_write = std::bind(iohandle::file::write,param::_1, param::_2);
		instance->_read = [=](char* buffer, const size_t buffer_size) {
			return iohandle::file::read(drive, entire_dir, buffer, buffer_size, *seek);
		};
		return instance;
	}
	static IOHandle* sys(const SYS_Type sys_type) { 
		auto instance = new IOHandle();
		instance->_read = [=](char* buffer, const size_t buffer_size) {
			switch (sys_type) {
				case SYS_Type::PROCFS: return iohandle::sys::procfs(buffer, buffer_size);
				default: return size_t();
			}
		};
		return instance;
	}

	size_t write(char* buffer, size_t buffer_size) {
		return _write(buffer, buffer_size);
	}
	
	size_t read(char* buffer, const size_t buffer_size) {
		return _read(buffer, buffer_size);
	}
};

static const std::map<std::string, IOHandle*> SYS_HANDLERS = {
	{"/procfs", IOHandle::sys(SYS_Type::PROCFS)}
};