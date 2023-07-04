#pragma once

#include "core.h"

namespace limbo
{
	enum class TextureType : uint8
	{
		Texture1D,
		Texture2D,
		Texture3D,
        TextureCube
	};

    enum class ShaderType : uint8
    {
	    Graphics,
        Compute
    };

	enum class Format : uint8
	{
        // R
        R8_UNORM,
        R8_UINT,
        R16_UNORM,
        R16_UINT,
        R16_SFLOAT,
        R32_UINT,
        R32_SFLOAT,
        // RG
        RG8_UNORM,
        RG16_SFLOAT,
        RG32_SFLOAT,
        // RGB
        B10GR11_UFLOAT_PACK32,
        RGB32_SFLOAT,
        // RGBA
        RGBA8_UNORM,
        A2RGB10_UNORM_PACK32,
        RGBA16_UNORM,
        RGBA16_SNORM,
        RGBA16_SFLOAT,
        RGBA32_SFLOAT,
        // DEPTH
        D16_UNORM,
        D32_SFLOAT,
        D32_SFLOAT_S8_UINT,
        // Compressed
        BC7_UNORM_BLOCK,
        ASTC_4x4_UNORM_BLOCK,
        //Surface
        BGRA8_UNORM,

        MAX
	};
}
