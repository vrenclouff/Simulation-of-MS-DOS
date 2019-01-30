#pragma once

#include "fat.h"

#include <cstdint>
#include <ctime>
#include <string>

// @author: Lukas Cerny

namespace fat_tool {

	struct File {
		std::string name;
		std::string extension;
	};

	File parse_entire_name(std::string file_name);
	uint16_t to_date(const std::tm tm);
	uint16_t to_time(const std::tm tm);
	bool is_attr(const uint8_t attributes, kiv_os::NFile_Attributes attribute);
	std::tm makedate(const uint16_t date, const uint16_t time);
	std::string& rtrim(std::string &s);
}