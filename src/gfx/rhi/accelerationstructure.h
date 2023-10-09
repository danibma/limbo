#pragma once

#include "resourcemanager.h"

namespace limbo::Gfx
{
	class Scene;
}
namespace limbo::RHI
{
	class CommandContext;
	class AccelerationStructure
	{
	private:
		BufferHandle	m_ScratchBuffer;
		BufferHandle	m_TLAS;
		BufferHandle	m_InstancesBuffer;

	public:
		AccelerationStructure() = default;
		~AccelerationStructure();

		void Build(CommandContext* cmd, const std::vector<Gfx::Scene*>& scenes);

		Buffer* GetTLASBuffer() const;
	};
}
