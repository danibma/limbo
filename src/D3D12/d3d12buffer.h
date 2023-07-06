#pragma once

#include "buffer.h"

namespace limbo::rhi
{
	class D3D12Buffer : public Buffer
	{
	public:
		D3D12Buffer() = default;
		D3D12Buffer(const BufferSpec& spec);

		virtual ~D3D12Buffer();
	};
}