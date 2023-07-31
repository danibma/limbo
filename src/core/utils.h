#pragma once

#include "core.h"

#include <string>

namespace limbo::Core
{
	class Window;
}

namespace limbo::Utils
{
	void StringConvert(const std::string& from, std::wstring& to);
	void StringConvert(const wchar_t* from, char* to);
	bool OpenFileDialog(Core::Window* window, const wchar_t* dialogTitle, std::vector<wchar_t*>& outResults, const wchar_t* defaultPath = L"", const std::vector<const wchar_t*>& extensions = {}, bool bMultipleSelection = false);

	inline uint64 ToKB(uint64 bytes)
	{
		return bytes * (1 << 10);
	}

	inline uint64 ToMB(uint64 bytes)
	{
		return bytes * (1 << 20);
	}

	inline uint64 ToGB(uint64 bytes)
	{
		return bytes * (1 << 30);
	}
}
