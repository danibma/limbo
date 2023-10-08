#pragma once
#include "buffer.h"

namespace limbo::Gfx
{
	class Scene;
}
namespace limbo::RHI
{
	class AccelerationStructure
	{
	private:
		BufferHandle	m_ScratchBuffer;
		BufferHandle	m_TLAS;
		BufferHandle	m_InstancesBuffer;

	public:
		AccelerationStructure() = default;
		~AccelerationStructure();

		void Build(const std::vector<Gfx::Scene*>& scenes);

		Buffer* GetTLASBuffer() const;
	};
}
