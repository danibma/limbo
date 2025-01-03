﻿#pragma once
#include "gfx/fpscamera.h"
#include "gfx/rhi/texture.h"
#include "gfx/techniques/rendertechnique.h"

namespace limbo::RHI
{
	class AccelerationStructure;
	class CommandContext;
}

namespace limbo::Gfx
{
	class RenderContext;
	class PathTracing final : public RenderTechnique
	{
	public:
		PathTracing();
		~PathTracing();

		virtual bool Init() override;
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) override;
		virtual void RenderUI(RenderContext& context) override;

	private:
		PathTracerConstants m_Constants;
		RHI::TextureHandle m_AccumulationBuffer; 
	};
}
