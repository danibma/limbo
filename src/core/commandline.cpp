#include "stdafx.h"
#include "commandline.h"

namespace limbo::Core
{
	namespace Win32Console
	{
		static HANDLE Open()
		{
			HANDLE handle = NULL;
			if (AllocConsole())
			{
				// Redirect the CRT standard input, output, and error handles to the console
				FILE* pCout;
				freopen_s(&pCout, "CONIN$", "r", stdin);
				freopen_s(&pCout, "CONOUT$", "w", stdout);
				freopen_s(&pCout, "CONOUT$", "w", stderr);

				handle = GetStdHandle(STD_OUTPUT_HANDLE);

				//Disable Close-Button
				HWND hwnd = GetConsoleWindow();
				if (hwnd != nullptr)
				{
					HMENU hMenu = GetSystemMenu(hwnd, FALSE);
					if (hMenu != nullptr)
					{
						DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
					}
				}
			}
			return handle;
		}

		static void Close(HANDLE handle)
		{
			if (handle)
				CloseHandle(handle);
		}
	};

	static std::vector<std::string> s_CommandLineArgs;
	HANDLE CommandLine::ConsoleHandle;

	void CommandLine::Init(const char* args)
	{
#if !NO_LOG
		ConsoleHandle = Win32Console::Open();
#endif

		size_t charNum = strlen(args);
		std::string arg = "";
		for (size_t i = 0; i <= charNum; ++i)
		{
			if (args[i] == ' ')
			{
				s_CommandLineArgs.emplace_back(arg);
				arg.clear();
				continue;
			}

			if (i == charNum)
			{
				s_CommandLineArgs.emplace_back(arg);
				continue;
			}

			arg += args[i];
		}
	}

	bool CommandLine::HasArg(const char* arg)
	{
		for (size_t i = 0; i < s_CommandLineArgs.size(); ++i)
		{
			if (s_CommandLineArgs[i] == arg)
				return true;
		}

		return false;
	}

	void CommandLine::Parse(const char* arg, std::string& value)
	{
		value = "";

		size_t argSize = strlen(arg);
		for (size_t i = 0; i < s_CommandLineArgs.size(); ++i)
		{
			std::string_view currentArg = s_CommandLineArgs[i];

			if (currentArg.size() <= argSize)
				continue;

			for (size_t stringIndex = 0; stringIndex < currentArg.size(); ++stringIndex)
			{
				if (stringIndex == argSize && currentArg[stringIndex] == '=')
					value = currentArg.substr(stringIndex + 1);

				if (currentArg[stringIndex] != arg[stringIndex])
					break;
			}
		}
	}

	CommandLine::~CommandLine()
	{
#if !NO_LOG
		Win32Console::Close(ConsoleHandle);
#endif
	}
}
