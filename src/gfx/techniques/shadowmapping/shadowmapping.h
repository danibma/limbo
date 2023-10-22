#pragma once
#include "gfx/shaderinterop.h"
#include "gfx/rhi/texture.h"
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	struct FPSCamera;

	class ShadowMapping : public RenderTechnique
	{
		ShadowData					m_ShadowData;
		RHI::TextureHandle			m_DepthShadowMaps[SHADOWMAP_CASCADES];
		float						m_CascadeSplitLambda = 0.95f;

	public:
		ShadowMapping();
		virtual ~ShadowMapping() override;

		virtual bool Init() override;
		virtual bool ConditionalRender(RenderContext& context) override;
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) override;

	private:
		void DrawDebugWindow();
		void CreateLightMatrices(const FPSCamera* camera, float3 lightDirection);
	};
}
