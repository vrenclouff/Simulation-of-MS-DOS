#include "parser.h"
#include "../api/api.h"
#include "rtl.h"

#include <vector>

bool parsePart(char* args, kiv_os::THandle stdin_handle, kiv_os::THandle stdout_handle) {

	char* arguments;
	char* tofunc = strtok_s(args, " ", &arguments);

	if (tofunc == NULL) {
		return NULL;
	}

	size_t origsize = strlen(tofunc);
	size_t size = origsize + sizeof(char);
	char* function = (char*)malloc(size);
	strncpy_s(function, size, tofunc, size);

	// TODO protoze api/user.def - asi vyresit lip
	if (!strcmp(function, "find")) {
		function = "wc";
	}
	else if (!strcmp(function, "tasklist")) {
		function = "ps";
	}
	else if (!strcmp(function, "wc") || !strcmp(function, "ps")) {
		function = "";
	}

	kiv_os::THandle handlers[1];
	handlers[0] = kiv_os_rtl::Clone(function, arguments, stdin_handle, stdout_handle);
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

void parse(char* args, kiv_os::THandle shellout, size_t shellcounter) {

	std::vector<char*> parts;

	const char* pipesymbol = "|";
	const char* redirectionsymbol = ">";
	char* rest;
	char* part = strtok_s(args, pipesymbol, &rest);

	while (part != NULL) {
		parts.push_back(part);
		part = strtok_s(NULL, pipesymbol, &rest);
	}

	kiv_os::THandle stdin_handle = 0;

	kiv_os::THandle* handles = NULL;

	std::vector<char*>::iterator it;
	for (it = parts.begin(); it != parts.end(); it++) {

		kiv_os::THandle stdout_handle = 1;

		// bool hasnext = (it != (parts.end() - 1));
		bool hasnext = false;

		if (hasnext) {
			handles = kiv_os_rtl::Create_Pipe();
			stdout_handle = *handles;

			if (strstr(*it, redirectionsymbol) != NULL) {
				wrongRedirection(shellout, shellcounter, stdin_handle, stdout_handle);
				return;
			}

			parsePart(*it, stdin_handle, stdout_handle);
		}
		else {

			if (strstr(*it, redirectionsymbol) != NULL) {
				char* command = strtok_s(*it, redirectionsymbol, &rest);
				char* filename = strtok_s(NULL, redirectionsymbol, &rest);

				if (strtok_s(NULL, redirectionsymbol, &rest) != NULL) {
					wrongRedirection(shellout, shellcounter, stdin_handle, stdout_handle);
					return;
				}

				kiv_os::THandle filehandle;
				kiv_os_rtl::Open_File(filename, sizeof(filename), filehandle);

				parsePart(command, stdin_handle, filehandle);
				kiv_os_rtl::Close_Handle(filehandle);
			}
			else {
				parsePart(*it, stdin_handle, stdout_handle);
			}
			

		}

		kiv_os_rtl::Close_Handle(stdin_handle);
		kiv_os_rtl::Close_Handle(stdout_handle);

		if (hasnext) {
			stdin_handle = *(handles + 1);
		}
		
	}

}
