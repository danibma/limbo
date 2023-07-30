#include "stdafx.h"
#include "commandline.h"

namespace limbo::core
{
	namespace Win32Console
	{
		static HANDLE open()
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

		static void close(HANDLE handle)
		{
			if (handle)
				CloseHandle(handle);
		}
	};

	static std::vector<std::string> s_commandLineArgs;
	HANDLE CommandLine::m_consoleHandle;

	void CommandLine::init(const char* args)
	{
		m_consoleHandle = Win32Console::open();

		size_t charNum = strlen(args);
		std::string arg = "";
		for (size_t i = 0; i <= charNum; ++i)
		{
			if (args[i] == ' ')
			{
				s_commandLineArgs.emplace_back(arg);
				arg.clear();
				continue;
			}

			if (i == charNum)
			{
				s_commandLineArgs.emplace_back(arg);
				continue;
			}

			arg += args[i];
		}
	}

	bool CommandLine::hasArg(const char* arg)
	{
		for (size_t i = 0; i < s_commandLineArgs.size(); ++i)
		{
			if (s_commandLineArgs[i] == arg)
				return true;
		}

		return false;
	}

	void CommandLine::parse(const char* arg, std::string& value)
	{
		value = "";

		size_t argSize = strlen(arg);
		for (size_t i = 0; i < s_commandLineArgs.size(); ++i)
		{
			std::string_view currentArg = s_commandLineArgs[i];

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
		Win32Console::close(m_consoleHandle);
	}
}
