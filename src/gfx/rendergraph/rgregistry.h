#pragma once

#include "rgresources.h"

namespace limbo::Gfx
{
	class RGRegistry
	{
	public:
		RGRegistry() = default;

		RGHandle Create(const RGTexture::TSpec& spec)
		{
			RGHandle result;
			result.Index = 0;
			return result;
		}

		RGHandle Create(const RGBuffer::TSpec& spec)
		{
			RGHandle result;
			result.Index = 0;
			return result;
		}
	};
}