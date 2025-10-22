//Copyright(C) 2025 Lost Empire Entertainment
//This program comes with ABSOLUTELY NO WARRANTY.
//This is free software, and you are welcome to redistribute it under certain conditions.
//Read LICENSE.md for more information.

#include "KalaHeaders/log_utils.hpp"
#include "KalaHeaders/string_utils.hpp"

#include "command.hpp"

using KalaHeaders::Log;
using KalaHeaders::LogType;
using KalaHeaders::ContainsString;
using KalaHeaders::RemoveAllFromString;

namespace CLI
{
	bool CommandManager::ParseCommand(const vector<string>& params)
	{
		if (params.empty()) return false;

		if (!COMMAND_PREFIX.empty()
			&& !ContainsString(params[0], COMMAND_PREFIX.data()))
		{
			Log::Print(
				"Target command '" + params[0] + "' is missing required prefix '" + COMMAND_PREFIX.data() + "'!",
				"PARSE",
				LogType::LOG_ERROR,
				2);

				return false;
		}

		vector<string> cleanedParams = params;

		Command foundCommand{};

		if (!COMMAND_PREFIX.empty()) cleanedParams[0] = RemoveAllFromString(cleanedParams[0], COMMAND_PREFIX.data());

		for (const auto& c : commands)
		{
			if (find(c.primary.begin(), c.primary.end(), cleanedParams[0]) != c.primary.end()
				&& cleanedParams.size() == c.paramCount)
			{
				foundCommand = c;

				break;
			}
		}

		if (foundCommand.primary.empty()
			&& foundCommand.paramCount == 0
			&& !foundCommand.targetFunction)
		{
			Log::Print(
				"Inserted command '" + cleanedParams[0] + "' does not exist!",
				"PARSE",
				LogType::LOG_ERROR,
				2);

			return false;
		}

		if (!foundCommand.targetFunction)
		{
			Log::Print(
				"Target command '" + cleanedParams[0] + "' is nullptr!",
				"PARSE",
				LogType::LOG_ERROR,
				2);

			return false;
		}

		foundCommand.targetFunction(cleanedParams);

		return true;
	}
}