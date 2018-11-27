#include "commands.h"
#include "rtl.h"
#include "error.h"

#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <array>
#include "commands.h"

#define REDIRECT	">"
#define PIPE		"|"

std::string normalize_process_name(const std::string& process_name) {
	if (process_table.find(process_name) != process_table.end()) {
		return process_table[process_name];
	}
	return process_name;
}

bool parse_cmd(const std::string& cmd_line, const kiv_os::THandle std_in, const kiv_os::THandle std_out, cmd::Error& error) {
	const struct Command { std::string name; std::string param; };
	std::vector<Command> programs;
	std::string to_file;
	std::istringstream iss(cmd_line);
	bool is_was_fnc = false, is_redirect = false;
	for (auto str = std::istream_iterator<std::string>(iss); str != std::istream_iterator<std::string>(); str++) {
		const auto& item = *str;
		if (item.compare(PIPE) == 0) { is_was_fnc = false; continue; }
		if (item.compare(REDIRECT) == 0) { is_redirect = true; continue; }
		if (is_redirect) { to_file = item; break; }
		if (is_was_fnc) { programs.back().param.append(item).append(" "); continue; }
		programs.push_back({ item, "" });
		is_was_fnc = true;
	}

	auto in = std_in;
	auto out = std_out;

	std::vector<std::array<kiv_os::THandle, 2>> pipes;
	std::vector<kiv_os::THandle> pids;
	for (auto program_itr = programs.begin(); program_itr != programs.end(); program_itr++) {
		const auto& program = *program_itr;
		if (program_itr == programs.end() - 1 && !to_file.empty()) {
			// open the file for the redirection
			if (!kiv_os_rtl::Open_File(to_file.data(), to_file.size(), out, false, static_cast<kiv_os::NFile_Attributes>(0))) {
				error = { Error_Message(kiv_os_rtl::Last_Error) };
				return false;
			}
		}
		if ((program_itr + 1) != programs.end()) {
			// create the pipe
			std::array<kiv_os::THandle, 2> pipe;
			if (!kiv_os_rtl::Create_Pipe(pipe.data())) {
				error = { Error_Message(kiv_os_rtl::Last_Error) };
				return false;
			}
			out = pipe[0];
			pipes.push_back(pipe);
		}

		// create the program
		kiv_os::THandle pid;
		const auto program_name = normalize_process_name(program.name);
		const auto program_params = program.param.empty() ? program.param : program.param.substr(0, program.param.size() - 1);
		if (embedded_processes.find(program_name) != embedded_processes.end()) {
			return embedded_processes[program_name](program_params, in, out, error);
		} else if (!kiv_os_rtl::Clone(pid, program_name.data(), program_params.data(), in, out)) {
			error = { "Invalid command.\n" };
			return false;
		}
		in = !pipes.empty() ? pipes.back()[1] : std_in;
		out = std_out;
		pids.push_back(pid);
	}

	size_t pid_index = 0;
	size_t pids_size = pids.size();
	for (const auto& pid : pids) {
		kiv_os::THandle th[] = { pid };
		kiv_os_rtl::Wait_For(th);
		// close read pipe handle
		if (pid_index > 0) {
			kiv_os_rtl::Close_Handle(pipes[pid_index - 1][0]);
		}
		// clse write pipe handle
		if (pid_index != pids_size - 1) {
			kiv_os_rtl::Close_Handle(pipes[pid_index][1]);
		}
		kiv_os_rtl::Read_Exit_Code(pid);
		pid_index++;
	}

	return true;
}

bool cd(const std::string& param, const kiv_os::THandle in, const kiv_os::THandle out, cmd::Error& error) {

	if (!kiv_os_rtl::Set_Working_Dir(param.data())) {
		error = { Error_Message(kiv_os_rtl::Last_Error) };
		return false;
	}

	return false;
}
