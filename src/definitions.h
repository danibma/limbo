#pragma once

#include "core.h"

namespace limbo
{
    constexpr uint8	NUM_BACK_BUFFERS = 3;

	enum class TextureType : uint8
	{
		Texture1D,
		Texture2D,
		Texture3D,
        TextureCube
	};

    enum class TextureUsage : uint8
    {
        Sampled             = 0x00000001,
    	Storage             = 0x00000002,
        Transfer_Dst        = 0x00000004,
        Transfer_Src        = 0x00000008,
        Color_Attachment    = 0x00000016,
        Depth_Attachment    = 0x00000032
    };
    inline TextureUsage operator|(TextureUsage lhs, TextureUsage rhs)
    {
        return static_cast<TextureUsage>(static_cast<uint8>(lhs) | static_cast<uint8>(rhs));
    }
    inline TextureUsage operator&(TextureUsage lhs, TextureUsage rhs)
    {
        return static_cast<TextureUsage>(static_cast<uint8>(lhs) & static_cast<uint8>(rhs));
    }

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
