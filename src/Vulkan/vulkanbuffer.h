#pragma once

#include "buffer.h"

namespace limbo
{
	class VulkanBuffer : public Buffer
	{
	public:
		VulkanBuffer() = default;
		VulkanBuffer(const BufferSpec& spec);

		virtual ~VulkanBuffer();
	};
}