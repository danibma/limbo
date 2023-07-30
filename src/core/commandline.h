#pragma once

namespace limbo::core
{
	class CommandLine
	{
	public:
		CommandLine() = default;
		~CommandLine();


		static void init(const char* args);
		static bool hasArg(const char* arg);
		static void parse(const char* arg, std::string& value);

	private:
		static HANDLE m_consoleHandle;
	};
}
