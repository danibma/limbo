﻿#pragma once
#include "resourcepool.h"

namespace limbo::Gfx
{
	class Buffer;
	class Scene;
	class AccelerationStructure
	{
	private:
		Handle<Buffer>		m_ScratchBuffer;
		Handle<Buffer>		m_TLAS;

	public:
		AccelerationStructure() = default;
		~AccelerationStructure();

		void Build(const std::vector<Scene*>& scenes);

		D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptor() const;
	};
}
