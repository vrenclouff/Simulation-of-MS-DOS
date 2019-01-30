#include "fs_tools.h"

#include <filesystem>

// @author: Lukas Cerny

namespace fs = std::filesystem;

std::string fs_tool::to_absolute_path(std::string base_path, std::string path) {
	if (fs::u8path(path).is_absolute()) {
		return path;
	}
	auto fsbasepath = fs::u8path(base_path);
	fsbasepath /= path;
	return fsbasepath.lexically_normal().string();
}

void fs_tool::to_absolute_components(std::vector<std::string>& components, std::string path) {
	for (const auto& item : std::filesystem::path(path)) {
		components.push_back(item.string());
	}
	if (components.back().empty()) {
		components.pop_back();
	}
}