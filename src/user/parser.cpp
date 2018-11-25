#include "parser.h"
#include "../api/api.h"
#include "rtl.h"

#include <vector>
#include <iostream>
#include <sstream>

std::string trim(std::string& str) {
	str.erase(0, str.find_first_not_of(' '));
	str.erase(str.find_last_not_of(' ') + 1);
	return str;
}

bool parse_Part(std::string args, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle) {

	std::istringstream is(args);
	std::string tofunc;
	std::string arguments;

	std::getline(is, tofunc, ' ');
	std::getline(is, arguments);

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
		const std::string error = "Invalid command.\n";
		kiv_os_rtl::Write_File(stdout_handle, error.c_str(), error.length(), counter);
		return 1;
	}

	kiv_os_rtl::Wait_For(handlers);
	kiv_os_rtl::Read_Exit_Code(handlers[0]);
	return 0;
}

void wrong_Redirection(kiv_os::THandle shellout, size_t shellcounter, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle) {
	kiv_os_rtl::Close_Handle(stdin_handle);
	kiv_os_rtl::Close_Handle(stdout_handle);
	const std::string error = "Redirections are not allowed at these places.\n";
	kiv_os_rtl::Write_File(shellout, error.c_str(), error.length(), shellcounter);
}

void parse(char* args, kiv_os::THandle shellin, kiv_os::THandle shellout, size_t shellcounter) {

	std::istringstream cdis(args);
	std::string cd;
	std::string dirname;
	char space = ' ';

	std::getline(cdis, cd, space);

	if (cd.find("cd") != std::string::npos)
	{
		std::getline(cdis, dirname, space);
		if (!kiv_os_rtl::Set_Working_Dir(dirname.c_str())) {
			const auto error_msg = std::string_view("Directory doesn't exist.\n");
			size_t wrote;
			kiv_os_rtl::Write_File(shellout, error_msg.data(), error_msg.size(), wrote);
		}
		return;
	}

	std::vector<std::string> parts;

	const char pipesymbol = '|';
	const char redirectionsymbol = '>';

	std::istringstream is(args);
	std::string part;
	while (std::getline(is, part, pipesymbol)) {
		parts.push_back(part);
	}

	kiv_os::THandle stdin_handle = shellin;

	kiv_os::THandle pipehandles[2];

	for (int i = 0; i < parts.size(); i++) {

		std::string curr = trim(parts.at(i));

		kiv_os::THandle stdout_handle = shellout;

		bool hasnext = (i != (parts.size() - 1));

		if (hasnext) {
			if (!kiv_os_rtl::Create_Pipe(pipehandles)) {
				// TODO error 
			}

			stdout_handle = pipehandles[0];

			if (curr.find(redirectionsymbol) != std::string::npos) {
				wrong_Redirection(shellout, shellcounter, stdin_handle, stdout_handle);
				return;
			}

			// TODO zasekne se to na Wait_For a spusti se jen jeden program
			parse_Part(curr, stdin_handle, stdout_handle);
		}
		else {

			if (curr.find(redirectionsymbol) != std::string::npos) {

				std::istringstream is(curr);
				std::string command;
				std::string filename;
				std::string rest;

				std::getline(is, command, redirectionsymbol);
				std::getline(is, filename, redirectionsymbol);

				command = trim(command);
				filename = trim(filename);

				if (std::getline(is, rest, redirectionsymbol)) {
					wrong_Redirection(shellout, shellcounter, stdin_handle, stdout_handle);
					return;
				}

				kiv_os::THandle filehandle;
				kiv_os_rtl::Open_File(filename.c_str(), filename.size(), filehandle, false, static_cast<kiv_os::NFile_Attributes>(0));

				parse_Part(command, stdin_handle, filehandle);
				kiv_os_rtl::Close_Handle(filehandle);
			}
			else {
				parse_Part(curr, stdin_handle, stdout_handle);
			}
		
		}
	
		//kiv_os_rtl::Close_Handle(stdin_handle);
		//kiv_os_rtl::Close_Handle(stdout_handle);

		if (hasnext) {
			stdin_handle = pipehandles[1];
		}
		
	}

}
