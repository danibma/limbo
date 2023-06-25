#pragma once

#include "core.h"

namespace limbo
{
	struct BufferSpec
	{
		const char* debugName = nullptr;

		uint32 byteSize = 0;
		const void* initialData = nullptr;
	};

	class Buffer
	{

	};
}