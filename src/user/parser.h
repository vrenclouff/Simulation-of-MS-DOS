#pragma once

#include "../api/api.h"

bool parsePart(char* args, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle);

void parse(char* args);
