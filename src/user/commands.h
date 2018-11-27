#pragma once

#include "error.h"

#include <string>
#include <map>
#include <vector>
#include <functional>

std::string& normalize_process_name(std::string& process_name);
bool parse_cmd(const std::string& cmd_line, const kiv_os::THandle std_in, const kiv_os::THandle std_out, cmd::Error& error);

bool cd(const std::string& param, const kiv_os::THandle in, const kiv_os::THandle out, cmd::Error& error);


static std::map<std::string, std::string> process_table = {
	{"tasklist", "ps"},
	{"find", "wc"},
};

static std::map<std::string, std::function<bool(const std::string& param, const kiv_os::THandle in, const kiv_os::THandle out, cmd::Error& error)>> embedded_processes = {
	{"cd", cd}
};
