﻿#pragma once

#define LIMBO_CMD_PROFILE "--profile"
#define LIMBO_CMD_WAIT_FOR_DEBUGGER "--waitfordebugger"

namespace limbo::Core
{
	class CommandLine
	{
	public:
		CommandLine() = default;
		~CommandLine();


		static void Init(const char* args);
		static bool HasArg(const char* arg);
		static void Parse(const char* arg, std::string& value);

		static HANDLE ConsoleHandle;
	};
}
