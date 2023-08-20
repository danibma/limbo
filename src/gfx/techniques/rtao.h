#pragma once

#include "gfx/rhi/resourcepool.h"

namespace limbo::Gfx
{
	class AccelerationStructure;
	class SceneRenderer;
	class Texture;
	class Shader;
	class RTAO
	{
		Handle<Shader>				m_RTAOShader;
		Handle<Shader>				m_DenoiseRTAOShader;
		Handle<Texture>				m_NoisedTexture;
		Handle<Texture>				m_FinalTexture;
		Handle<Texture>				m_PreviousFrame;

		uint32						m_AccumCount;

	public:
		explicit RTAO();
		~RTAO();

		void Render(SceneRenderer* sceneRenderer, AccelerationStructure* sceneAS, Handle<Texture> positionsMap, Handle<Texture> normalsMap);

		Handle<Texture> GetFinalTexture() const;

	private:
		void PreparePreviousFrameTexture();
	};
}

