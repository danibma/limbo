#pragma once

#include "gfx/rhi/texture.h"

namespace limbo::RHI { class CommandContext; }

namespace limbo::Gfx
{
	class SceneRenderer;
	class SSAO
	{
		RHI::TextureHandle			m_UnblurredSSAOTexture;
		RHI::TextureHandle			m_BlurredSSAOTexture;

	public:
		explicit SSAO();
		~SSAO();

		void Render(RHI::CommandContext* cmd, SceneRenderer* sceneRenderer, RHI::TextureHandle positionsMap, RHI::TextureHandle sceneDepthMap);

		RHI::TextureHandle GetBlurredTexture() const;
	};
}
