#pragma once
#include "gfx/shaderinterop.h"
#include "gfx/rhi/resourcepool.h"

namespace limbo::RHI
{
	struct Shader;
	class RootSignature;
	class PipelineStateObject;
	class Texture;
	class Buffer;
}

namespace limbo::Gfx
{
	struct FPSCamera;

	class SceneRenderer;
	class ShadowMapping
	{
		RHI::RootSignature*			m_CommonRS;
		RHI::Handle<RHI::Shader>	m_ShadowMapVS;
		RHI::Handle<RHI::Shader>	m_ShadowMapPS;
		RHI::Handle<RHI::Texture>	m_DepthShadowMaps[SHADOWMAP_CASCADES];
		RHI::PipelineStateObject*	m_PSO;

		ShadowData					m_ShadowData;

		float						m_CascadeSplitLambda = 0.95f;

	public:
		explicit ShadowMapping();
		~ShadowMapping();

		void Render(SceneRenderer* sceneRenderer);
		void DrawDebugWindow();

		const ShadowData& GetShadowData() const;

	private:
		void CreateLightMatrices(const FPSCamera* camera, float3 lightDirection);
	};
}
