#pragma once

#include <stdio.h>

namespace limbo
{
	inline void Log(const char* message)
	{
		printf(message);
	}
}