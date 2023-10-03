#pragma once
#include "gfx/fpscamera.h"
#include "gfx/rhi/accelerationstructure.h"
#include "gfx/rhi/resourcepool.h"

namespace limbo::RHI
{
	struct Shader;
	class RootSignature;
	class PipelineStateObject;
	class Texture;
}

namespace limbo::Gfx
{
	class SceneRenderer;
	class PathTracing
	{
	private:
		RHI::RootSignature*				m_CommonRS;
		RHI::Handle<RHI::Texture>		m_FinalTexture;
		RHI::Handle<RHI::Shader>		m_PathTracerLib;
		RHI::Handle<RHI::Shader>		m_MaterialLib;
		RHI::PipelineStateObject*		m_PSO;

	public:
		explicit PathTracing();
		~PathTracing();

		void Render(SceneRenderer* sceneRenderer, RHI::AccelerationStructure* sceneAS, const FPSCamera& camera);

		RHI::Handle<RHI::Texture> GetFinalTexture() const;
	};
}
