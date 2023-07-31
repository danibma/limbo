#pragma once

#include "core.h"

#include <string>

namespace limbo::Utils
{
	void StringConvert(const std::string& from, std::wstring& to);
	void StringConvert(const wchar_t* from, char* to);

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
