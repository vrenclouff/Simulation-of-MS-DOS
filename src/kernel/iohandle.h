#pragma once

#include "fat.h"
#include "circular_buffer.h"

#include <map>
#include <functional>
#include <mutex>

enum class SYS_Type : uint8_t {
	PROCFS = 1,
};


enum Permission : uint8_t {
	Read = 1,
	Write = 2,
};

static const std::map<std::string, SYS_Type> SYS_TABLE = {
	{"/procfs", SYS_Type::PROCFS}
};


class IOHandle {
private:
	const uint8_t _permission;
protected:
	std::mutex _io_mutex;
public:
	virtual ~IOHandle() {}
	IOHandle() : _permission(Permission::Read) {}
	IOHandle(const uint8_t permission) : _permission(permission) {}
	virtual size_t read(char* buffer, const size_t buffer_size) { return 0; }
	virtual size_t write(char* buffer, const size_t buffer_size) { return 0; }
	virtual void close() {}
	void check_ACL(Permission acl) {
		if (!(_permission & acl)) {
			throw kiv_os::NOS_Error::Permission_Denied;
		}
	}
};

class IOHandle_Keyboard : public IOHandle {
public:
	IOHandle_Keyboard() : IOHandle(Permission::Read) {}
	virtual size_t read(char* buffer, const size_t buffer_size) final override;
	virtual size_t write(char* buffer, const size_t buffer_size) final override {
		IOHandle::check_ACL(Permission::Write); return 0;
	}
};

class IOHandle_VGA : public IOHandle {
public:
	IOHandle_VGA() : IOHandle(Permission::Write) {}
	virtual size_t read(char* buffer, const size_t buffer_size) final override {
		IOHandle::check_ACL(Permission::Read); return 0;
	}
	virtual size_t write(char* buffer, const size_t buffer_size) final override;
};

class IOHandle_File : public IOHandle {
private:
	const kiv_fs::Drive_Desc _drive;
	kiv_fs::File_Desc _file;

	size_t seek = 0;

public:
	IOHandle_File(const kiv_fs::Drive_Desc drive, const kiv_fs::File_Desc file, const uint8_t permission) :
		IOHandle(permission), _drive(drive), _file(file) {}
	virtual size_t read(char* buffer, const size_t buffer_size) final override;
	virtual size_t write(char* buffer, const size_t buffer_size) final override;
};

class IOHandle_SYS : public IOHandle {

private:
	const SYS_Type _type;

public:
	IOHandle_SYS(const SYS_Type type) : _type(type) {}
	virtual size_t read(char* buffer, const size_t buffer_size) final override;
	virtual size_t write(char* buffer, const size_t buffer_size) final override {
		IOHandle::check_ACL(Permission::Write); return 0;
	}
};

class IOHandle_Pipe : public IOHandle {

private:
	std::shared_ptr<Circular_buffer> _circular_buffer;

public:
	IOHandle_Pipe(std::shared_ptr<Circular_buffer> circular_buffer, const uint8_t permission) : IOHandle(permission), _circular_buffer(circular_buffer) {}

	virtual size_t read(char* buffer, const size_t buffer_size) final override;
	virtual size_t write(char* buffer, const size_t buffer_size) final override;
	virtual void close() final override;
};