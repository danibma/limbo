#pragma once
#include "gfx/fpscamera.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/texture.h"
#include "gfx/rhi/pipelinestateobject.h"

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
		RHI::RootSignatureHandle	m_CommonRS;
		RHI::TextureHandle			m_FinalTexture;
		RHI::ShaderHandle			m_PathTracerLib;
		RHI::ShaderHandle			m_MaterialLib;
		RHI::PSOHandle				m_PSO;

	public:
		explicit PathTracing();
		~PathTracing();

		void Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, const FPSCamera& camera);

		RHI::TextureHandle GetFinalTexture() const;
	};
}
