#pragma once

#include <cstdint>
#include <ctime>
#include <string>

// for print_sector
#include <sstream>
#include <iomanip>
#include <array>
#include <functional>

namespace fat_tool {

	struct File {
		std::string name;
		std::string extension;
	};

	File parse_file_name(std::string file_name);
	uint16_t to_date(const std::tm tm);
	uint16_t to_time(const std::tm tm);
	std::tm makedate(const uint16_t date, const uint16_t time);
	std::string& rtrim(std::string &s);


	void print_sector(size_t sector_num, void* sector, std::function<void(const char*)> print_str);
}