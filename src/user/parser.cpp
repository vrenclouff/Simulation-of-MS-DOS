#include "parser.h"
#include "../api/api.h"
#include "rtl.h"

#include <vector>
#include <iostream>
#include <sstream>

bool parsePart(std::string args, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle) {

	std::istringstream is(args);
	std::string tofunc;
	std::string arguments;
	std::string rest;

	std::getline(is, tofunc, ' ');
	std::getline(is, arguments, ' ');

	if (tofunc.empty()) {
		return NULL;
	}

	std::string function(tofunc);

	// TODO protoze api/user.def - asi vyresit lip
	if (!function.compare("find")) {
		function = "wc";
	}
	else if (!function.compare("tasklist")) {
		function = "ps";
	}
	else if (!function.compare("wc") || !function.compare("ps")) {
		function = "";
	}

	kiv_os::THandle handlers[1];
	handlers[0] = kiv_os_rtl::Clone(function.c_str(), arguments.c_str(), stdin_handle, stdout_handle);

	if (handlers[0] == NULL)
	{
		size_t counter;
		const char* error = "Invalid command.\n";
		kiv_os_rtl::Write_File(stdout_handle, error, strlen(error), counter);
		return 1;
	}

	kiv_os_rtl::Wait_For(handlers);
	kiv_os_rtl::Read_Exit_Code(handlers[0]);
	return 0;
}

void wrongRedirection(kiv_os::THandle shellout, size_t shellcounter, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle) {
	kiv_os_rtl::Close_Handle(stdin_handle);
	kiv_os_rtl::Close_Handle(stdout_handle);
	const char* error = "Redirections are not allowed at these places.\n";
	kiv_os_rtl::Write_File(shellout, error, strlen(error), shellcounter);
}

void parse(char* args, kiv_os::THandle shellin, kiv_os::THandle shellout, size_t shellcounter) {

	char* first = "";
	char* dirname;
	size_t inputsize = strlen(args) + 1;
	char* tokenize = new char[inputsize];

	strcpy_s(tokenize, strlen(tokenize), args);
	first = strtok_s(tokenize, " ", &dirname);

	if (!strcmp(first, "cd"))
	{
		if (!kiv_os_rtl::Set_Working_Dir(dirname)) {
			const auto error_msg = std::string_view("Directory doesn't exist.\n");
			size_t wrote;
			kiv_os_rtl::Write_File(shellout, error_msg.data(), error_msg.size(), wrote);
		}
		return;
	}

	std::vector<std::string> parts;

	const char pipesymbol = '|';
	const char redirectionsymbol = '>';

	std::istringstream is(args);	// TODO zde to nahodne pada
	std::string part;
	while (std::getline(is, part, pipesymbol)) {
		parts.push_back(part);
	}

	kiv_os::THandle stdin_handle = shellin;

	kiv_os::THandle* handles = NULL;

	for (int i = 0; i < parts.size(); i++) {

		std::string curr = parts.at(i);

		kiv_os::THandle stdout_handle = shellout;

		bool hasnext = (i != (parts.size() - 1));

		if (hasnext) {
			handles = kiv_os_rtl::Create_Pipe();
			stdout_handle = *handles;

			if (curr.find(redirectionsymbol) != std::string::npos) {
				wrongRedirection(shellout, shellcounter, stdin_handle, stdout_handle);
				return;
			}

			parsePart(curr, stdin_handle, stdout_handle);
		}
		else {

			if (curr.find(redirectionsymbol) != std::string::npos) {

				std::istringstream is(curr);
				std::string command;
				std::string filename;
				std::string rest;

				std::getline(is, command, redirectionsymbol);
				std::getline(is, filename, redirectionsymbol);
				
				if (std::getline(is, rest, redirectionsymbol)) {
					wrongRedirection(shellout, shellcounter, stdin_handle, stdout_handle);
					return;
				}

				kiv_os::THandle filehandle;
				kiv_os_rtl::Open_File(filename.c_str(), filename.size(), filehandle, false, kiv_os::NFile_Attributes::Read_Only);

				parsePart(command, stdin_handle, filehandle);
				kiv_os_rtl::Close_Handle(filehandle);
			}
			else {
				parsePart(curr, stdin_handle, stdout_handle);
			}
			

		}

		//kiv_os_rtl::Close_Handle(stdin_handle);
		//kiv_os_rtl::Close_Handle(stdout_handle);

		if (hasnext) {
			stdin_handle = *(handles + 1);
		}
		
	}

}

void getErrorMessage(kiv_os::NOS_Error error, char* message) {

	switch (error) {

	case (kiv_os::NOS_Error::Directory_Not_Empty):
		message = "Directory is not empty.\n";
		break;
	case (kiv_os::NOS_Error::File_Not_Found):
		message = "File not found.\n";
		break;
	case (kiv_os::NOS_Error::Invalid_Argument):
		message = "Invalid argument.\n";
		break;
	case (kiv_os::NOS_Error::IO_Error):
		message = "I/O error occured.\n";
		break;
	case (kiv_os::NOS_Error::Not_Enough_Disk_Space):
		message = "Not enough disk space.\n";
		break;
	case (kiv_os::NOS_Error::Out_Of_Memory):
		message = "Out of memmory.\n";
		break;
	case (kiv_os::NOS_Error::Permission_Denied):
		message = "Permission denied.\n";
		break;
	default:
		message = "Something went wrong.\n";
		break;
	}

}
