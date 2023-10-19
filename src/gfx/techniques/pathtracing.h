#pragma once
#include "gfx/fpscamera.h"
#include "gfx/rhi/texture.h"

namespace limbo::RHI
{
	class AccelerationStructure;
	class CommandContext;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class PathTracing
	{
	private:
		RHI::TextureHandle			m_FinalTexture;

	public:
		explicit PathTracing();
		~PathTracing();

		void Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, const FPSCamera& camera);

		RHI::TextureHandle GetFinalTexture() const;
	};
}
