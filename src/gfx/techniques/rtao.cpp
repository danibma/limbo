#include "stdafx.h"
#include "rtao.h"

#include "gfx/rhi/device.h"
#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx
{
	RTAO::RTAO()
	{
		m_FinalTexture = CreateTexture({
			.Width = GetBackbufferWidth(),
			.Height = GetBackbufferHeight(),
			.DebugName = "RTAO Final Texture",
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = Format::RGBA8_UNORM,
			.Type = TextureType::Texture2D,
		});

		m_RTAOShader = CreateShader({
			.ProgramName = "RTAO",
			.Libs = {
				{
					.LibName = "raytracing/rtao",
					.Exports = {
						{ .Name = L"RTAORayGen" },
					}
				},
			},
			.ShaderConfig = {
				.MaxPayloadSizeInBytes = sizeof(float), // AOPayload
				.MaxAttributeSizeInBytes = sizeof(float2) // float2 barycentrics
			},
			.Type = ShaderType::RayTracing,
		});
	}

	RTAO::~RTAO()
	{
		DestroyTexture(m_FinalTexture);
		DestroyShader(m_RTAOShader);
	}

	void RTAO::Render(SceneRenderer* sceneRenderer)
	{
	}

	Handle<Texture> RTAO::GetFinalTexture() const
	{
		return m_FinalTexture;
	}
}
