#pragma once

#include "core.h"

namespace limbo::algo
{
    // sdbm hash function - http://www.cse.yorku.ca/~oz/hash.html
	static uint32 hash(const char* str)
	{
        FAILIF(!str, ~0);

        uint32 hash = 0;
        int c;

        while ((c = *str++))
            hash = c + (hash << 6) + (hash << 16) - hash;

        return hash;
	}
}
