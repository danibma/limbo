#pragma once
#include "gfx/fpscamera.h"
#include "gfx/rhi/accelerationstructure.h"
#include "gfx/rhi/resourcepool.h"

namespace limbo::Gfx
{
	class SceneRenderer;
	class Texture;
	class Shader;
	class PathTracing
	{
	private:
		Handle<Texture>			m_FinalTexture;
		Handle<Shader>			m_RTShader;

	public:
		explicit PathTracing();
		~PathTracing();

		void Render(SceneRenderer* sceneRenderer, AccelerationStructure* sceneAS, const FPSCamera& camera);

		Handle<Texture> GetFinalTexture() const;
	};
}
