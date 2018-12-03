#include "io_manager.h"

#include "fat_tools.h"
#include "drive.h"
#include "fs_tools.h"

#include <filesystem>
#include <mutex>

std::mutex _io_mutex;

bool io::is_exist_dir(std::string path) {
	std::lock_guard<std::mutex> locker(_io_mutex);

	std::vector<std::string> components;
	fs_tool::to_absolute_components(components, path);
	kiv_fs::Drive_Desc drive = drive::at(components.front());
	components.erase(components.begin());
	std::vector<kiv_fs::File_Desc> files;
	const auto res = kiv_fs::find_entire_dirs(drive, files, components);
	return res && fat_tool::is_attr(files.back().entire_dir.attributes, kiv_os::NFile_Attributes::Directory);
}

IOHandle* Open_Exist_File(const std::string& drive_volume, const std::vector<std::string> components, const kiv_os::NFile_Attributes attributes, kiv_os::NOS_Error& error) {
	if (drive_volume.compare("A:") == 0) {
		std::string sys;
		for (const auto& cmp : components) {
			sys += cmp;
		}
		return new IOHandle_SYS(SYS_TABLE.at(sys));
	}
	else {
		const auto drive = drive::at(drive_volume);
		std::vector<kiv_fs::File_Desc> files;
		if (!kiv_fs::find_entire_dirs(drive, files, components)) {
			error = kiv_os::NOS_Error::IO_Error;
			return false;
		}
		const auto read_only = fat_tool::is_attr(static_cast<uint8_t>(attributes), kiv_os::NFile_Attributes::Read_Only);
		const auto is_root = components.size() == 1;
		return new IOHandle_File(drive, files.back(), read_only, !is_root ? files.rbegin()[1].sectors : std::vector<uint16_t>(0));
	}
}

bool Rewrite_File(kiv_fs::File_Desc& new_file, const std::string& filename, const std::vector<kiv_fs::File_Desc>& file_stack, kiv_os::NOS_Error& error) {

	new_file = file_stack.back();

	if (fat_tool::is_attr(new_file.entire_dir.attributes, kiv_os::NFile_Attributes::Directory)) {
		error = kiv_os::NOS_Error::Directory_Not_Empty;
		return false;
	}

	std::string name = filename;
	std::transform(filename.begin(), filename.end(), name.begin(), ::toupper);
	if (std::string(new_file.entire_dir.file_name).compare(name) != 0) {
		error = kiv_os::NOS_Error::File_Not_Found;
		return false;
	}

	new_file.sectors.erase(new_file.sectors.begin() + 1, new_file.sectors.end());
	new_file.entire_dir.size = 0;

	return true;
}

bool Create_File(kiv_fs::File_Desc& new_file, const kiv_fs::Drive_Desc& drive, const std::string& filename, const kiv_os::NFile_Attributes attributes, const std::vector<kiv_fs::File_Desc>& file_stack, kiv_os::NOS_Error& error) {

	auto disk_status = kiv_hal::NDisk_Status::No_Error;

	new_file.sectors = std::vector<uint16_t>();
	if (!kiv_fs::new_entire_dir(new_file.entire_dir, filename, static_cast<uint8_t>(attributes), disk_status)) {
		error = kiv_os::NOS_Error::IO_Error;
		return false;
	}

	std::vector<std::div_t> fat_offsets;
	if (!kiv_fs::find_free_sectors(fat_offsets, drive.id, START_OF_FAT, 1, drive.boot_block.bytes_per_sector, disk_status)) {
		error = kiv_os::NOS_Error::IO_Error;
		return false;
	}

	// save a new directory to the FAT
	if (!kiv_fs::save_to_fat(drive.id, fat_offsets, drive.boot_block.bytes_per_sector, kiv_fs::offset(drive.boot_block), new_file.sectors, new_file.entire_dir.first_cluster, disk_status)) {
		error = kiv_os::NOS_Error::IO_Error;
		return false;
	}

	const auto& parent = file_stack.back();
	// save to parent folder entire_dir for a new folder
	if (!kiv_fs::save_to_dir(drive.id, parent.sectors, drive.boot_block.bytes_per_sector, new_file.entire_dir, kiv_fs::Edit_Type::Add, disk_status)) {
		error = kiv_os::NOS_Error::IO_Error;
		return false;
	}

	if (file_stack.size() > 1) {
		auto grandparent = file_stack.end()[-2];
		grandparent.entire_dir.size += sizeof kiv_fs::FATEntire_Directory;
		if (!kiv_fs::save_to_dir(drive.id, grandparent.sectors, drive.boot_block.bytes_per_sector, parent.entire_dir, kiv_fs::Edit_Type::Edit, disk_status)) {
			error = kiv_os::NOS_Error::IO_Error;
			return false;
		}
	}

	return true;
}

IOHandle* Open_New_File(const std::string& drive_volume, const std::vector<std::string> components, const kiv_os::NFile_Attributes attributes, kiv_os::NOS_Error& error) {

	auto disk_status = kiv_hal::NDisk_Status::No_Error;

	std::vector<kiv_fs::File_Desc> files;
	const auto drive = drive::at(drive_volume);
	const auto is_exist = kiv_fs::find_entire_dirs(drive, files, components);

	if (files.size() != (components.size() - (is_exist ? 0 : 1))) {
		error = kiv_os::NOS_Error::IO_Error;
		return nullptr;
	}

	kiv_fs::File_Desc new_file;
	kiv_fs::File_Desc parent_file;

	
	if (is_exist) {
		parent_file = files.end()[-2];
		if (!Rewrite_File(new_file, components.back(), files, error)) {
			return nullptr;
		}
	}
	else {
		parent_file = files.back();
		if (!Create_File(new_file, drive, components.back(), attributes, files, error)) {
			return nullptr;
		}
	}

	const auto is_newfile_dir = fat_tool::is_attr(new_file.entire_dir.attributes, kiv_os::NFile_Attributes::Directory);
	const auto is_read_only = fat_tool::is_attr(new_file.entire_dir.attributes, kiv_os::NFile_Attributes::Read_Only);
	uint8_t permission = is_read_only ? Permission::Read : Permission::Read | Permission::Write;

	return is_newfile_dir ? new IOHandle() : new IOHandle_File(drive, new_file, permission, is_read_only ? std::vector<uint16_t>(0) : parent_file.sectors);
}

IOHandle* io::Open_File(std::string absolute_path, const kiv_os::NOpen_File fm, const kiv_os::NFile_Attributes attributes, kiv_os::NOS_Error& error) {
	std::lock_guard<std::mutex> locker(_io_mutex);

	std::vector<std::string> components;
	fs_tool::to_absolute_components(components, absolute_path);

	const auto drive_volume = components.front();
	components.erase(components.begin());

	if (fm == kiv_os::NOpen_File::fmOpen_Always) {
		return Open_Exist_File(drive_volume, components, attributes, error);
	}
	else {
		return Open_New_File(drive_volume, components, attributes, error);
	}
}

bool io::Remove_File(std::string absolute_path, kiv_os::NOS_Error& error) {
	std::lock_guard<std::mutex> locker(_io_mutex);

	std::vector<std::string> components;
	fs_tool::to_absolute_components(components, absolute_path);

	const auto drive = drive::at(components.front());
	components.erase(components.begin());
	auto disk_status = kiv_hal::NDisk_Status::No_Error;

	std::vector<kiv_fs::File_Desc> files;
	if (!kiv_fs::find_entire_dirs(drive, files, components)) {
		error = kiv_os::NOS_Error::File_Not_Found;
		return false;
	}

	const auto file_to_dell = files.back();
	files.pop_back();

	if (!fat_tool::is_attr(file_to_dell.entire_dir.attributes, kiv_os::NFile_Attributes::Directory)) {
		error = kiv_os::NOS_Error::Invalid_Argument;
		return false;
	}

	if (file_to_dell.entire_dir.size) {
		error = kiv_os::NOS_Error::Directory_Not_Empty;
		return false;
	}

	// free space in FAT
	kiv_fs::remove_sectors_in_fat(file_to_dell.entire_dir, drive.boot_block.bytes_per_sector, drive.id, disk_status);

	if (disk_status != kiv_hal::NDisk_Status::No_Error) {
		error = kiv_os::NOS_Error::IO_Error;
		return false;
	}

	// find parrent to remove Entry_dir
	auto parrent_file = files.back();
	files.pop_back();

	if (!kiv_fs::save_to_dir(drive.id, parrent_file.sectors, drive.boot_block.bytes_per_sector, file_to_dell.entire_dir, kiv_fs::Edit_Type::Del, disk_status)) {
		error = kiv_os::NOS_Error::IO_Error;
		return false;
	}

	if (files.size() > 1) {
		// find grandparent
		const auto grandparrent_file = files.back();
		files.pop_back();

		parrent_file.entire_dir.size -= sizeof kiv_fs::FATEntire_Directory;
		if (!kiv_fs::save_to_dir(drive.id, grandparrent_file.sectors, drive.boot_block.bytes_per_sector, parrent_file.entire_dir, kiv_fs::Edit_Type::Edit, disk_status)) {
			error = kiv_os::NOS_Error::IO_Error;
			return false;
		}
	}

	return true;
}