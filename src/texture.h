#pragma once

#include "definitions.h"

namespace limbo
{
	struct TextureSpec
	{
		uint32		width;
		uint32		height;
		Format		format  = Format::R8_UNORM;
		TextureType type	= TextureType::Texture2D;
	};

	class Texture
	{
	public:
		virtual ~Texture() = default;
	};
}
