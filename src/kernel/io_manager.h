#pragma once

#include "fat.h"

#include <functional>

class IOManager {
public:
	const kiv_fs::FATBoot_Block& _boot_block;
	std::function<void(unsigned char* data, uint16_t sector, kiv_hal::NDisk_IO operation)> _hard_disk;

	IOManager(
		const kiv_fs::FATBoot_Block& boot_block,
		std::function<void(unsigned char* data, uint16_t sector, kiv_hal::NDisk_IO operation)> hard_disk
	) : _boot_block(boot_block), _hard_disk(hard_disk) {}

	uint8_t open();
	void close();
};