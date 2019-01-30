#pragma once

#include "../api/api.h"

#include <string>

// @author: Lukas Cerny

namespace cmd {
	struct Error {
		std::string what;
	};
}

std::string Error_Message(kiv_os::NOS_Error error);