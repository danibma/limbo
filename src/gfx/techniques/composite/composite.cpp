#include "stdafx.h"
#include "composite.h"

#include "gfx/psocache.h"
#include "gfx/rendercontext.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/rhi/device.h"

namespace limbo::Gfx
{
	namespace 
	{
		int32 s_CurrentTonemap = (int32)Tonemap::GT;
		bool  s_EnableGammaCorrection = true;
	}

	Composite::Composite()
		: RenderTechnique("Scene Composite")
	{
	}

	bool Composite::Init()
	{
		return true;
	}

	void Composite::Render(RHI::CommandContext& cmd, RenderContext& context)
	{
		if (!ensure(context.SceneTextures.PreCompositeSceneTexture.IsValid()))
			context.SceneTextures.PreCompositeSceneTexture = RHI::ResourceManager::Ptr->EmptyTexture;

		cmd.BeginProfileEvent(m_Name.data());
		cmd.SetPipelineState(PSOCache::Get(PipelineID::Composite));
		cmd.SetPrimitiveTopology();
		cmd.SetRenderTargets(RHI::GetCurrentBackbuffer(), RHI::GetCurrentDepthBackbuffer());
		cmd.SetViewport(RHI::GetBackbufferWidth(), RHI::GetBackbufferHeight());

		cmd.ClearRenderTargets(RHI::GetCurrentBackbuffer());
		cmd.ClearDepthTarget(RHI::GetCurrentDepthBackbuffer());

		cmd.BindConstants(0, 0, s_CurrentTonemap);
		cmd.BindConstants(0, 1, s_EnableGammaCorrection);
		cmd.BindConstants(0, 2, RM_GET(context.SceneTextures.PreCompositeSceneTexture)->SRV());

		cmd.Draw(6);
		cmd.EndProfileEvent(m_Name.data());
	}

	void Composite::RenderUI(RenderContext& context)
	{
		if (ImGui::TreeNode("Composite"))
		{
			const char* tonemapList[ENUM_COUNT<Tonemap>()] = { "None", "AcesFilm", "Reinhard", "Uncharted2", "Unreal", "GT", "Agx" };
			ImGui::Combo("Tonemap", &s_CurrentTonemap, tonemapList, ENUM_COUNT<Tonemap>());
			ImGui::Checkbox("Gamma Correction", &s_EnableGammaCorrection);

			ImGui::TreePop();
		}
	}
}
