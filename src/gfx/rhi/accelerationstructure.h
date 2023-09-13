#pragma once
#include "resourcepool.h"

namespace limbo::Gfx
{
	class Scene;
}
namespace limbo::RHI
{
	class Buffer;
	class AccelerationStructure
	{
	private:
		Handle<Buffer>		m_ScratchBuffer;
		Handle<Buffer>		m_TLAS;
		Handle<Buffer>		m_InstancesBuffer;

	public:
		AccelerationStructure() = default;
		~AccelerationStructure();

		void Build(const std::vector<Gfx::Scene*>& scenes);

		D3D12_GPU_DESCRIPTOR_HANDLE GetDescriptor() const;
	};
}
