#pragma once
#include "gfx/fpscamera.h"
#include "gfx/rhi/accelerationstructure.h"
#include "gfx/rhi/resourcepool.h"

namespace limbo::Gfx
{
	class Texture;
	class Shader;
	class PathTracing
	{
	private:
		Handle<Texture>			m_FinalTexture;
		Handle<Shader>			m_RTShader;

		AccelerationStructure	m_AccelerationStructure;

	public:
		explicit PathTracing();
		~PathTracing();

		void Render(const FPSCamera& camera);
		void RebuildAccelerationStructure(const std::vector<Scene*>& scenes);

		Handle<Texture> GetFinalTexture() const;
	};
}
