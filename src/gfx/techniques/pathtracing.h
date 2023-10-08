#pragma once
#include "gfx/fpscamera.h"
#include "gfx/rhi/accelerationstructure.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/texture.h"
#include "gfx/rhi/pipelinestateobject.h"

namespace limbo::RHI
{
	class RootSignature;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class PathTracing
	{
	private:
		RHI::RootSignature*	m_CommonRS;
		RHI::TextureHandle	m_FinalTexture;
		RHI::ShaderHandle	m_PathTracerLib;
		RHI::ShaderHandle	m_MaterialLib;
		RHI::PSOHandle		m_PSO;

	public:
		explicit PathTracing();
		~PathTracing();

		void Render(SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, const FPSCamera& camera);

		RHI::TextureHandle GetFinalTexture() const;
	};
}
