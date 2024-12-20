﻿#pragma once
#include "gfx/shaderinterop.h"
#include "gfx/rhi/texture.h"
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	struct FPSCamera;

	class ShadowMapping : public RenderTechnique
	{
		RHI::TextureHandle			m_ShadowMaps[SHADOWMAP_CASCADES];
		float						m_CascadeSplitLambda = 0.95f;

	public:
		ShadowMapping();
		virtual ~ShadowMapping() override;

		virtual bool Init() override;
		virtual bool ConditionalRender(RenderContext& context) override;
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) override;
		virtual void RenderUI(RenderContext& context) override;

	private:
		void DrawDebugWindow();
		void CreateLightMatrices(RenderContext& context);
	};
}
