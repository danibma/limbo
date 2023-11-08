#pragma once
#include "gfx/psocache.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx::TechniqueHelpers
{
	class BlurPass
	{
	public:
		static void Execute(RHI::CommandContext& cmd, RHI::TextureHandle input, RHI::TextureHandle output, uint2 textureSize, const std::string_view& profileName)
		{
			auto intermediate = RHI::CreateTexture(RM_GET(input)->Spec);

			// Horizontal
			{
				std::string name = std::format("{} Blur Horizontal Pass", profileName);
				cmd.BeginProfileEvent(name.c_str());
				cmd.SetPipelineState(PSOCache::Get(PipelineID::Blur_H));

				cmd.BindConstants(0, 0, RM_GET(input)->SRV());

				RHI::DescriptorHandle uavHandles[] =
				{
					RM_GET(intermediate)->UAVHandle[0]
				};
				cmd.BindTempDescriptorTable(1, uavHandles, ARRAY_LEN(uavHandles));

				cmd.Dispatch(Math::DivideAndRoundUp(textureSize.x, 512u), textureSize.y, 1);
				cmd.EndProfileEvent(name.c_str());
			}

			// Vertical
			{
				std::string name = std::format("{} Blur Vertical Pass", profileName);
				cmd.BeginProfileEvent(name.c_str());
				cmd.SetPipelineState(PSOCache::Get(PipelineID::Blur_V));

				cmd.BindConstants(0, 0, RM_GET(intermediate)->SRV());

				RHI::DescriptorHandle uavHandles[] =
				{
					RM_GET(output)->UAVHandle[0]
				};
				cmd.BindTempDescriptorTable(1, uavHandles, ARRAY_LEN(uavHandles));

				cmd.Dispatch(textureSize.x, Math::DivideAndRoundUp(textureSize.y, 512u), 1);
				cmd.EndProfileEvent(name.c_str());
			}

			RHI::DestroyTexture(intermediate);
		}
	};
}
