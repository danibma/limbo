#pragma once

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

	private:
		static HANDLE m_ConsoleHandle;
	};
}
