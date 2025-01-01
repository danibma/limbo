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
	bool PathExists(const wchar_t* path);
	bool PathExists(const char* path);

	bool FileRead(const char* filename, std::vector<uint8>& filedata);

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

namespace limbo::Random
{
	inline uint32 PCG_Hash(uint32 seed)
	{
		uint32 state = seed * 747796405u + 2891336453u;
		uint32 word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
		return (word >> 22u) ^ word;
	}

	inline float Float(uint32 seed)
	{
		uint32 value = PCG_Hash(seed);
		return (float)value / (float)UINT_MAX;
	}

	inline float Float(uint32 seed, float min, float max)
	{
		return Float(seed) * (max - min) + min;
	}
}
