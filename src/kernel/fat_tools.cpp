
#include "fat_tools.h"
#include <algorithm>
#include <string>

// @author: Lukas Cerny

fat_tool::File fat_tool::parse_entire_name(std::string file_name) {
	std::transform(file_name.begin(), file_name.end(), file_name.begin(), ::toupper);
	const auto pos = file_name.find_last_of(".");

	if (pos == std::string::npos) {
		return { file_name, "" };
	}

	const auto extension = file_name.substr(pos + 1);
	const auto name = file_name.substr(0, file_name.size() - extension.size() - 1);
	return { name, extension };
}

uint16_t fat_tool::to_date(const std::tm tm) {
	return (tm.tm_year - 80) << 9 | (tm.tm_mon + 1) << 5 | tm.tm_mday;
}

uint16_t fat_tool::to_time(const std::tm tm) {
	return tm.tm_hour << 11 | tm.tm_min << 5 | tm.tm_sec / 0x2;
}

bool fat_tool::is_attr(const uint8_t attributes, kiv_os::NFile_Attributes attribute) {
	return attributes & static_cast<uint8_t>(attribute);
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