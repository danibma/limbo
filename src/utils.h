#pragma once
#include <string>

namespace limbo::utils
{
	void StringConvert(const std::string& from, std::wstring& to);
	void StringConvert(const std::wstring& from, std::string& to);
}
