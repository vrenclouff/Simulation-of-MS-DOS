
#include "fat_tools.h"
#include <algorithm>
#include <string>

fat_tool::File fat_tool::parse_file_name(std::string file_name) {
	std::transform(file_name.begin(), file_name.end(), file_name.begin(), ::toupper);
	const auto extension = file_name.substr(file_name.find_last_of(".") + 1);
	const auto name = file_name.substr(0, file_name.size() - extension.size() - 1);
	return { name, extension };
}

uint16_t fat_tool::to_date(const std::tm tm) {
	return (tm.tm_year - 80) << 9 | (tm.tm_mon + 1) << 5 | tm.tm_mday;
}

uint16_t fat_tool::to_time(const std::tm tm) {
	return tm.tm_hour << 11 | tm.tm_min << 5 | tm.tm_sec / 0x2;
}

std::tm fat_tool::makedate(const uint16_t date, const uint16_t time) {
	std::tm tm = {};
	tm.tm_mday = date & 0x07;
	tm.tm_mon = ((date & 0x01E0) >> 5) - 1;
	tm.tm_year = ((date & 0xFE00) >> 9) + 80;
	tm.tm_hour = (time & 0xF800) >> 11;
	tm.tm_min = (time & 0x07E0) >> 5;
	tm.tm_sec = (time & 0x001F) * 2;
	return tm;
}

std::string& fat_tool::rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return ch >= 48 && ch <= 122; // 0 - z
	}).base(), s.end());
	return s;
}

void fat_tool::print_sector(size_t sector_num, void* sector, std::function<void(const char*)> print_str) {
	print_str("Block ");
	print_str(std::to_string(sector_num).c_str());
	print_str("\n");
	print_str("\t0\t1\t2\t3\t4\t5\t6\t7\t8\t9");
	print_str("\ta\tb\tc\td\te\tf");
	print_str("\n");
	const auto arr = static_cast<const char*>(sector);
	for (uint8_t i = 0; i < 32; i++) {
		print_str(std::to_string(i).c_str());
		char line[17]; line[16] = '\0';
		for (uint8_t j = 0; j < 16; j++) {
			print_str("\t");
			unsigned char val = arr[(i * 16) + j];
			line[j] = val == '\0' ? '.' : val;
			std::stringstream ss;
			ss << std::setfill('0') << std::setw(2) << std::hex << (int)val;
			print_str(ss.str().c_str());
		}
		print_str("\t");
		print_str(line);
		print_str("\n");
	}
}
