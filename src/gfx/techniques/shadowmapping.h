#pragma once
#include "gfx/shaderinterop.h"
#include "gfx/rhi/texture.h"

namespace limbo::RHI { class CommandContext; }

namespace limbo::Gfx
{
	struct FPSCamera;

	class SceneRenderer;
	class ShadowMapping
	{
		RHI::TextureHandle			m_DepthShadowMaps[SHADOWMAP_CASCADES];

		ShadowData					m_ShadowData;

		float						m_CascadeSplitLambda = 0.95f;

	public:
		explicit ShadowMapping();
		~ShadowMapping();

		void Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer);
		void DrawDebugWindow();

		const ShadowData& GetShadowData() const;

	private:
		void CreateLightMatrices(const FPSCamera* camera, float3 lightDirection);
	};
}
