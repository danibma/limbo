#include "stdafx.h"
#include "utils.h"

#include <windows.h>

namespace limbo::utils
{
	void StringConvert(const std::string& from, std::wstring& to)
	{
		int num = MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, NULL, 0);
		if (num > 0)
		{
			to.resize(size_t(num) - 1);
			MultiByteToWideChar(CP_UTF8, 0, from.c_str(), -1, &to[0], num);
		}
	}

	void StringConvert(const wchar_t* from, char* to)
	{
		int num = WideCharToMultiByte(CP_UTF8, 0, from, -1, NULL, 0, NULL, NULL);
		if (num > 0)
		{
			to[size_t(num) - 1] = '\0';
			WideCharToMultiByte(CP_UTF8, 0, from, -1, &to[0], num, NULL, NULL);
		}
	}
}
