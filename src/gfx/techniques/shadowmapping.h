#pragma once
#include "gfx/shaderinterop.h"
#include "gfx/rhi/shader.h"
#include "gfx/rhi/texture.h"
#include "gfx/rhi/pipelinestateobject.h"
#include "gfx/rhi/rootsignature.h"

namespace limbo::Gfx
{
	struct FPSCamera;

	class SceneRenderer;
	class ShadowMapping
	{
		RHI::RootSignatureHandle	m_CommonRS;
		RHI::ShaderHandle			m_ShadowMapVS;
		RHI::ShaderHandle			m_ShadowMapPS;
		RHI::TextureHandle			m_DepthShadowMaps[SHADOWMAP_CASCADES];
		RHI::PSOHandle				m_PSO;

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
