#pragma once

#include "core/core.h"

#include <d3d12/d3d12.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;

namespace limbo::Gfx
{
	constexpr uint8	NUM_BACK_BUFFERS = 3;

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
		Compute,
		RayTracing,
	};

	enum class ShaderParameterType : uint8
	{
		SRV = 0,
		UAV,
		CBV,
		Constants,
		MAX
	};

	enum class RenderPassOp : uint8
	{
		Clear,
		Load
	};

	enum class Format : uint8
	{
		UNKNOWN,

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
		RGBA8_UNORM_SRGB,
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
		//Surface
		BGRA8_UNORM
	};

	inline std::string_view ShaderParameterTypeToStr(ShaderParameterType type)
	{
		switch (type)
		{
		case ShaderParameterType::SRV: return "SRV";
		case ShaderParameterType::UAV: return "UAV";
		case ShaderParameterType::CBV: return "CBV";
		case ShaderParameterType::Constants: return "Constants";
		case ShaderParameterType::MAX:
		default: return "";
		}
	}

	inline D3D12_RESOURCE_DIMENSION D3DTextureType(TextureType type)
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

	inline uint8 D3DBytesPerChannel(Format format)
	{
		switch (format)
		{
		case Format::R8_UNORM:
		case Format::R8_UINT:
		case Format::RG8_UNORM:
		case Format::RGBA8_UNORM:
		case Format::RGBA8_UNORM_SRGB:
		case Format::BGRA8_UNORM:
			return 1;
		case Format::R16_UNORM:
		case Format::R16_UINT:
		case Format::R16_SFLOAT:
		case Format::D16_UNORM:
		case Format::RG16_SFLOAT:
		case Format::RGBA16_UNORM:
		case Format::RGBA16_SNORM:
		case Format::RGBA16_SFLOAT:
			return 2;
		case Format::R32_UINT:
		case Format::R32_SFLOAT:
		case Format::RG32_SFLOAT:
		case Format::RGB32_SFLOAT:
		case Format::RGBA32_SFLOAT:
		case Format::D32_SFLOAT:
			return 4;
		case Format::D32_SFLOAT_S8_UINT:
		case Format::BC7_UNORM_BLOCK:
		case Format::B10GR11_UFLOAT_PACK32:
		case Format::A2RGB10_UNORM_PACK32:
		case Format::UNKNOWN:
		default:
			ensure(false);
			return 0;
		}
	}

	inline DXGI_FORMAT D3DFormat(Format format)
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
		case Format::RGBA8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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
		case Format::BGRA8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Format::UNKNOWN:
		default: 
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	namespace Internal
	{
		void DXHandleError(HRESULT hr, const char* file, int line);
		void DXMessageCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext);
	}
}
#define DX_CHECK(expression) { HRESULT _hr = expression; if (_hr != S_OK) limbo::Gfx::Internal::DXHandleError(_hr, __FILE__, __LINE__); }
