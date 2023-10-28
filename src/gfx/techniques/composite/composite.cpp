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
		int32 s_CurrentTonemap = 1;
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

		cmd.BindConstants(0, 1, RM_GET(context.SceneTextures.PreCompositeSceneTexture)->SRV());

		cmd.Draw(6);
		cmd.EndProfileEvent(m_Name.data());
	}

	void Composite::RenderUI(RenderContext& context)
	{
		if (ImGui::TreeNode("Composite"))
		{
			enum class Tonemap : uint8
			{
				None = 0,
				AcesFilm,
				Reinhard,

				MAX
			};

			const char* tonemapList[ENUM_COUNT<Tonemap>()] = { "None", "AcesFilm", "Reinhard" };
			ImGui::Combo("Tonemap", &s_CurrentTonemap, tonemapList, ENUM_COUNT<Tonemap>());

			ImGui::TreePop();
		}
	}
}
