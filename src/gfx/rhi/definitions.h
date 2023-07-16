#pragma once

#include "core/core.h"
#include "definitions.h"

#include <d3d12/d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>

using namespace Microsoft::WRL;

namespace limbo::gfx
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
		Sampled = 0x00000001,
		Storage = 0x00000002,
		Transfer_Dst = 0x00000004,
		Transfer_Src = 0x00000008,
		Color_Attachment = 0x00000016,
		Depth_Attachment = 0x00000032
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

	inline D3D12_RESOURCE_DIMENSION d3dTextureType(TextureType type)
	{
		switch (type)
		{
		case TextureType::Texture1D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		case TextureType::TextureCube:
		case TextureType::Texture2D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case TextureType::Texture3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		default:
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		}
	}

	inline DXGI_FORMAT d3dFormat(Format format)
	{
		switch (format)
		{
		case Format::R8_UNORM:
			return DXGI_FORMAT_R8_UNORM;
		case Format::R8_UINT:
			return DXGI_FORMAT_R8_UINT;
		case Format::R16_UNORM:
			return DXGI_FORMAT_R16_UNORM;
		case Format::R16_UINT:
			return DXGI_FORMAT_R16_UINT;
		case Format::R16_SFLOAT:
			return DXGI_FORMAT_R16_FLOAT;
		case Format::R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case Format::R32_SFLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case Format::RG8_UNORM:
			return DXGI_FORMAT_R8G8_UNORM;
		case Format::RG16_SFLOAT:
			return DXGI_FORMAT_R16G16_FLOAT;
		case Format::RG32_SFLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		case Format::B10GR11_UFLOAT_PACK32:
			return DXGI_FORMAT_R11G11B10_FLOAT;
		case Format::RGB32_SFLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case Format::RGBA8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Format::A2RGB10_UNORM_PACK32:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case Format::RGBA16_UNORM:
			return DXGI_FORMAT_R16G16B16A16_UNORM;
		case Format::RGBA16_SNORM:
			return DXGI_FORMAT_R16G16B16A16_SNORM;
		case Format::RGBA16_SFLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Format::RGBA32_SFLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Format::D16_UNORM:
			return DXGI_FORMAT_D16_UNORM;
		case Format::D32_SFLOAT:
			return DXGI_FORMAT_D32_FLOAT;
		case Format::D32_SFLOAT_S8_UINT:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case Format::BC7_UNORM_BLOCK:
			return DXGI_FORMAT_BC7_UNORM;
		case Format::ASTC_4x4_UNORM_BLOCK:
			ensure(false);
			return DXGI_FORMAT_UNKNOWN; // No support in D3D12
		case Format::BGRA8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Format::MAX:
		default: 
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	namespace internal
	{
		void dxHandleError(HRESULT hr);
	}
}
#define DX_CHECK(expression) { HRESULT result = expression; if (result != S_OK) limbo::gfx::internal::dxHandleError(result); }
