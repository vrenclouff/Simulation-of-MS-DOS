#pragma once

#include <map>
#include <functional>

#include "fat.h"

enum class SYS_Type : uint8_t {
	PROCFS = 1,
};

class IOHandle {
public:
	virtual size_t read(char* buffer, size_t buffer_size) = 0;
	virtual size_t write(char* buffer, size_t buffer_size) = 0;
};

class IOHandle_Console : public IOHandle {
public:
	virtual size_t read(char* buffer, size_t buffer_size) final override;
	virtual size_t write(char* buffer, size_t buffer_size) final override;
};

class IOHandle_File : public IOHandle {
private:
	const kiv_fs::Drive_Desc _drive;
	kiv_fs::File_Desc _file;

	size_t seek = 0;

public:

	IOHandle_File(const kiv_fs::Drive_Desc drive, const kiv_fs::File_Desc file) : _drive(drive), _file(file) {}

	virtual size_t read(char* buffer, size_t buffer_size) final override;
	virtual size_t write(char* buffer, size_t buffer_size) final override;
};

class IOHandle_SYS : public IOHandle {

private:
	const SYS_Type _type;

public:
	IOHandle_SYS(const SYS_Type type) : _type(type) {}

	virtual size_t read(char* buffer, size_t buffer_size) final override;
	virtual size_t write(char* buffer, size_t buffer_size) final override { return 0; }
};

static const std::map<std::string, IOHandle*> SYS_HANDLERS = {
	{"/procfs", new IOHandle_SYS(SYS_Type::PROCFS)}
};
