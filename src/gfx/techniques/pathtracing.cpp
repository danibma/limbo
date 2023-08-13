#include "stdafx.h"
#include "pathtracing.h"

#include "gfx/rhi/resourcemanager.h"

namespace limbo::Gfx
{
	PathTracing::PathTracing()
	{
		m_FinalTexture = ResourceManager::Ptr->EmptyTexture;
	}

	PathTracing::~PathTracing()
	{
		DestroyTexture(m_FinalTexture);
	}

	void PathTracing::Render()
	{
	}

	Handle<Texture> PathTracing::GetFinalTexture() const
	{
		return m_FinalTexture;
	}
}
