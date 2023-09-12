#pragma once
#include "gfx/shaderinterop.h"
#include "gfx/rhi/resourcepool.h"

namespace limbo::Gfx
{
	struct FPSCamera;

	class SceneRenderer;
	class Texture;
	class Shader;
	class Buffer;
	class ShadowMapping
	{
		Handle<Shader>			m_ShadowMapShaders[SHADOWMAP_CASCADES];
		Handle<Buffer>			m_ShadowDataBuffer[NUM_BACK_BUFFERS]; // todo: this, along with the SceneInfo buffer should be temp allocated

		ShadowData				m_ShadowData;

		float					m_CascadeSplitLambda = 0.95f;

	public:
		explicit ShadowMapping();
		~ShadowMapping();

		void BindShadowMap(Handle<Shader> shader);

		void Render(SceneRenderer* sceneRenderer);
		void DrawDebugWindow();

	private:
		void CreateLightMatrices(const FPSCamera* camera, float3 lightDirection);
	};
}
