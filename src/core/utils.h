#pragma once
#include <string>

namespace limbo::utils
{
	void StringConvert(const std::string& from, std::wstring& to);
	void StringConvert(const wchar_t* from, char* to);
}
