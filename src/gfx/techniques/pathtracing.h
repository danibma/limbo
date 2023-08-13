#pragma once
#include "gfx/rhi/resourcepool.h"

namespace limbo::Gfx
{
	class Texture;

	class PathTracing
	{
	private:
		Handle<Texture>		m_FinalTexture;

	public:
		explicit PathTracing();
		~PathTracing();

		void Render();

		Handle<Texture> GetFinalTexture() const;
	};
}
