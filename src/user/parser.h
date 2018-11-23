#pragma once

#include "../api/api.h"

#include <string>

std::string trim(std::string const& str);

bool parsePart(std::string args, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle);

void wrongRedirection(kiv_os::THandle shellout, size_t shellcounter, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle);

void parse(char* args, kiv_os::THandle shellin, kiv_os::THandle shellout, size_t shellcounter);

std::string getErrorMessage(kiv_os::NOS_Error error);
