//Copyright(C) 2025 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/string_utils.hpp"
#include "KalaHeaders/file_utils.hpp"

#include "core.hpp"
#include "command.hpp"

using KalaHeaders::Log;
using KalaHeaders::LogType;
using KalaHeaders::SplitString;
using KalaHeaders::ListDirectoryContents;

using CLI::Core;
using CLI::Command;
using CLI::CommandManager;

using std::cin;
using std::getline;
using std::ostringstream;
using std::string;
using std::to_string;
using std::vector;
using std::filesystem::current_path;
using std::filesystem::path;

static void GetParams(int argc, char* argv[]);
static void WaitForInput();

static void AddBuiltInCommands();

//Built-in command for listing all commands
static void Command_Help(const vector<string>& params);
//Built-in command for listing info about chosen command
static void Command_Info(const vector<string>& params);

//Built-in command for listing current path
static void Command_Where(const vector<string>& params);
//Built-in command for listing all files and folders in current dir
static void Command_List(const vector<string>& params);
//Built-in command for going to desired path
static void Command_Go(const vector<string>& params);

//Built-in command for cleaning console commands
static void Command_Clear(const vector<string>& params);
//Built-in command for closing the cli
static void Command_Exit(const vector<string>& params);

namespace CLI
{
	void Core::Run(int argc, char* argv[])
	{
		GetParams(argc, argv);
		WaitForInput();
	}
}

void GetParams(int argc, char* argv[])
{
	if (argc == 1) WaitForInput();

	string insertedCommand{};

	vector<string> params{};
	for (int i = 1; i < argc; ++i)
	{
		params.emplace_back(argv[i]);
		insertedCommand += "'" + string(argv[i]) + "' ";
	}

	if (params.empty()) WaitForInput();

	Log::Print(
		"Inserted command: " + insertedCommand + "\n",
		"PARSE",
		LogType::LOG_INFO);

	CommandManager::ParseCommand(params);
}

void WaitForInput()
{
	AddBuiltInCommands();

	string line{};
	while (true)
	{
		Log::Print("\nEnter command:");

		getline(cin, line);

		//uncomment if you want each new command to clean the console
		//system("cls");

		vector<string> splitValue = SplitString(line, " ");

		if (splitValue.size() == 0) continue;

		CommandManager::ParseCommand(splitValue);
	}
}

void AddBuiltInCommands()
{
	static vector<string> empty{};

	Command cmd_help
	{
		.primary = { "help" },
		.description = "Lists all available commands.",
		.paramCount = 1,
		.targetFunction = Command_Help
	};
	Command cmd_info
	{
		.primary = { "info" },
		.description = "Lists info about chosen command.",
		.paramCount = 2,
		.targetFunction = Command_Info
	};
	Command cmd_clear
	{
		.primary = { "clear", "c" },
		.description = "Clears the console from all messages.",
		.paramCount = 1,
		.targetFunction = Command_Clear
	};

	Command cmd_where
	{
		.primary = { "where" },
		.description = "Displays current path.",
		.paramCount = 1,
		.targetFunction = Command_Where
	};
	Command cmd_list
	{
		.primary = { "list" },
		.description = "Lists all files and folders in current directory.",
		.paramCount = 1,
		.targetFunction = Command_List
	};
	Command cmd_go
	{
		.primary = { "go" },
		.description = "Goes to chosen directory.",
		.paramCount = 2,
		.targetFunction = Command_Go
	};

	Command cmd_exit
	{
		.primary = { "exit", "e" },
		.description = "Asks for user to press enter to close the cli, good for reading messages before quitting.",
		.paramCount = 1,
		.targetFunction = Command_Exit
	};
	Command cmd_qe
	{
		.primary = { "quickexit", "qe" },
		.description = "Quickly exits this cli without any 'Press Enter to quit' confirmation.",
		.paramCount = 1,
		.targetFunction = Command_Exit
	};

	CommandManager::AddCommand(cmd_help);
	CommandManager::AddCommand(cmd_info);

	CommandManager::AddCommand(cmd_where);
	CommandManager::AddCommand(cmd_list);
	CommandManager::AddCommand(cmd_go);

	CommandManager::AddCommand(cmd_clear);
	CommandManager::AddCommand(cmd_exit);
	CommandManager::AddCommand(cmd_qe);
}

void Command_Help(const vector<string>& params)
{
	ostringstream result{};

	result << "\nListing all commands. Type 'info' with a command name as the second parameter to get more info about that command\n";
	for (const auto& c : CommandManager::commands)
	{
		for (const auto& p : c.primary)
		{
			if (p == c.primary[0]) result << "  ";
			result << p;
			if (p != c.primary[c.primary.size() - 1])
			{
				result << ", ";
			}
			else result << "\n";
		}
	}

	Log::Print(result.str());
}

void Command_Info(const vector<string>& params)
{
	string command = params[1];

	ostringstream result{};

	result << "\n";

	Command cmd{};

	for (const auto& c : CommandManager::commands)
	{
		if (find(c.primary.begin(), c.primary.end(), command) != c.primary.end())
		{
			cmd = c;
			break;
		}
	}

	result << "primary variants: ";
	for (const auto& p : cmd.primary)
	{
		result << p;
		if (p != cmd.primary[cmd.primary.size() - 1])
		{
			result << ", ";
		}
		else result << "\n";
	}

	result << "description: " << cmd.description << "\n";
	result << "parameter count: " << to_string(cmd.paramCount) << "\n";

	Log::Print(result.str());
}

void Command_Where(const vector<string>& params)
{
	if (Core::currentDir.empty()) Core::currentDir = current_path().string();
	Log::Print("\nCurrently at: " + Core::currentDir);
}

void Command_List(const vector<string>& params)
{
	if (Core::currentDir.empty()) Core::currentDir = current_path().string();

	vector<path> content{};

	string result = ListDirectoryContents(Core::currentDir, content);

	if (!result.empty())
	{
		Log::Print(
			"Failed to list current directory contents! Reason: " + result,
			"COMMAND",
			LogType::LOG_ERROR,
			2);

		return;
	}

	ostringstream oss{};

	oss << "\nListing all paths at '" << Core::currentDir << "':\n";
	if (content.empty()) oss << "  - (empty)";
	else
	{
		for (size_t i = 0; i < content.size(); ++i)
		{
			oss << "  - ";

			path rel = content[i].lexically_relative(Core::currentDir);
			oss << rel.string();

			if (is_directory(content[i])) oss << "/";

			if (i + 1 < content.size()) oss << "\n";
		}
	}

	Log::Print(oss.str());
}

void Command_Go(const vector<string>& params)
{
	if (Core::currentDir.empty()) Core::currentDir = current_path().string();
	path correctTarget = weakly_canonical(path(Core::currentDir) / params[1]);

	if (!exists(correctTarget))
	{
		ostringstream oss{};
		oss << "Cannot go to target path '" << correctTarget
			<< "' because it does not exist!";

		Log::Print(
			oss.str(),
			"COMMAND",
			LogType::LOG_ERROR,
			2);

		return;
	}

	if (!is_directory(correctTarget))
	{
		ostringstream oss{};
		oss << "Cannot go to target path '" << correctTarget
			<< "' because it is not a directory!";

		Log::Print(
			oss.str(),
			"COMMAND",
			LogType::LOG_ERROR,
			2);

		return;
	}

	Core::currentDir = correctTarget.string();

	Log::Print("\nMoved to new path: " + Core::currentDir);
}

void Command_Clear(const vector<string>& params) { system("cls"); }

void Command_Exit(const vector<string>& params)
{
	if (params.size() == 1
		&& (params[0] == "exit"
			|| params[0] == "e"))
	{
		ostringstream out{};
		out << "\n==========================================================================================\n";
		Log::Print(out.str());

		Log::Print("Press 'Enter' to exit...");
		cin.get();
	}

	quick_exit(0);
}