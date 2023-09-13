#pragma once
#include "gfx/shaderinterop.h"
#include "gfx/rhi/resourcepool.h"

namespace limbo::RHI
{
	class Texture;
	class Shader;
	class Buffer;
}

namespace limbo::Gfx
{
	struct FPSCamera;

	class SceneRenderer;
	class ShadowMapping
	{
		RHI::Handle<RHI::Shader>	m_ShadowMapShaders[SHADOWMAP_CASCADES];
		RHI::Handle<RHI::Buffer>	m_ShadowDataBuffer[RHI::NUM_BACK_BUFFERS]; // todo: this, along with the SceneInfo buffer should be temp allocated

		ShadowData					m_ShadowData;

		float						m_CascadeSplitLambda = 0.95f;

	public:
		explicit ShadowMapping();
		~ShadowMapping();

		void BindShadowMap(RHI::Handle<RHI::Shader> shader);

		void Render(SceneRenderer* sceneRenderer);
		void DrawDebugWindow();

	private:
		void CreateLightMatrices(const FPSCamera* camera, float3 lightDirection);
	};
}
