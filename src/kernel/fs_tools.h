#pragma once

#include <vector>

// @author: Lukas Cerny

namespace fs_tool {
	void to_absolute_components(std::vector<std::string>& components, std::string path);
	std::string to_absolute_path(std::string base_path, std::string path);
}