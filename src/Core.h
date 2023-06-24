#pragma once

#include <stdio.h>

namespace limbo
{
	inline void Log(const char* message)
	{
#if LIMBO_WINDOWS
		printf("Windows: %s", message);
#elif LIMBO_LINUX
		printf("Linux: %s", message);
#endif
		
	}
}