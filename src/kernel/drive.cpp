#include "drive.h"

#include <mutex>

// @author: Lukas Cerny

std::mutex Drive_Guard;
std::map<std::string, kiv_fs::Drive_Desc> Drivers;


std::string drive::main_volume() {
	std::lock_guard<std::mutex> locker(Drive_Guard);

	return Drivers.begin()->first;
}

void drive::save(const std::string volume, const uint8_t id, const kiv_fs::FATBoot_Block & book_block) {
	std::lock_guard<std::mutex> locker(Drive_Guard);

	Drivers.insert(std::pair<std::string, kiv_fs::Drive_Desc>(volume, { id, book_block }));
}

kiv_fs::Drive_Desc drive::at(const std::string volume) {
	std::lock_guard<std::mutex> locker(Drive_Guard);

	if (Drivers.find(volume) != Drivers.end()) {
		return Drivers.at(volume);
	}
	return kiv_fs::Drive_Desc();
}
