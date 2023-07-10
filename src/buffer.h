#pragma once

#include "core.h"

namespace limbo
{
	enum class BufferUsage
	{
		Vertex,
		Index,
		Constant
	};

	struct BufferSpec
	{
		const char* debugName = nullptr;

		uint32		byteSize = 0;
		BufferUsage usage = BufferUsage::Constant;
		const void* initialData = nullptr;
	};

	class Buffer
	{
	public:
		virtual ~Buffer() = default;
	};
}