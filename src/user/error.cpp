#include "error.h"

std::string Error_Message(kiv_os::NOS_Error error) {
	switch (error) {
	case (kiv_os::NOS_Error::Directory_Not_Empty):   return "Directory is not empty.\n"; break;
	case (kiv_os::NOS_Error::File_Not_Found):	     return "File not found.\n";		break;
	case (kiv_os::NOS_Error::Invalid_Argument):	     return "Invalid argument.\n";		break;
	case (kiv_os::NOS_Error::IO_Error):			     return "I/O error occured.\n";		break;
	case (kiv_os::NOS_Error::Not_Enough_Disk_Space): return "Not enough disk space.\n";	break;
	case (kiv_os::NOS_Error::Out_Of_Memory):		 return "Out of memmory.\n";		break;
	case (kiv_os::NOS_Error::Permission_Denied):	 return "Permission denied.\n";		break;
	default:										 return "Something went wrong.\n";  break;
	}
}