#include "parser.h"
#include "../api/api.h"
#include "rtl.h"

bool parse(char* args) {

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
	handlers[0] = kiv_os_rtl::Clone(function, arguments);
	kiv_os_rtl::Wait_For(handlers);
	kiv_os_rtl::Read_Exit_Code(handlers[0]);
	return 0;
}
