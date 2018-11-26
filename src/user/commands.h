#pragma once

#include "error.h"

#include <string>
#include <map>
#include <vector>

namespace str {
	std::vector<std::string> split(const std::string& subject, const char delimeter);
}

static std::map<std::string, std::string> process_table = {
	{"tasklist", "ps"},
	{"find", "wd"},
};

std::string& normalize_process_name(std::string& process_name);
bool parse_cmd(const std::string& cmd_line, const kiv_os::THandle std_in, const kiv_os::THandle std_out, cmd::Error& error);