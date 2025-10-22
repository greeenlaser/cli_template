//Copyright(C) 2025 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include <iostream>
#include <string>
#include <vector>

#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/string_utils.hpp"

#include "core.hpp"
#include "command.hpp"

using KalaHeaders::Log;
using KalaHeaders::LogType;
using KalaHeaders::SplitString;

using CLI::Core;
using CLI::Command;
using CLI::CommandManager;

using std::cin;
using std::getline;
using std::ostringstream;
using std::string;
using std::to_string;
using std::vector;

static void GetParams(int argc, char* argv[]);
static void WaitForInput();

static void AddBuiltInCommands();

//Built-in commands lister
static void Command_Help(const vector<string>& params);
//Built-in command details lister
static void Command_Info(const vector<string>& params);
//Built-in console messages cleaner
static void Command_Clear(const vector<string>& params);
//Built-in cli closer
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
		Log::Print("Enter command:");

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

	result << "\nListing info about command:\n";

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