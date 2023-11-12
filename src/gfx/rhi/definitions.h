#pragma once

#include "core/core.h"

namespace limbo::RHI
{
	constexpr uint8	NUM_BACK_BUFFERS = 3;
	constexpr uint8 MAX_RENDER_TARGETS = 8;

	DECLARE_MULTICAST_DELEGATE(TOnPrepareFrame);
	inline TOnPrepareFrame OnPrepareFrame;

	DECLARE_MULTICAST_DELEGATE(TOnResizedSwapchain, uint32, uint32);
	inline TOnResizedSwapchain OnResizedSwapchain;

	DECLARE_MULTICAST_DELEGATE(TReloadShaders);
	inline TReloadShaders OnReloadShaders;

	enum class ContextType : uint8
	{
		Direct = 0,
		Copy,
		Compute,

		MAX
	};

	enum class ShaderType : uint8
	{
		Compute,
		Vertex,
		Mesh,
		Pixel,
		Lib,

		MAX
	};

	enum class TextureType : uint8
	{
		Texture1D,
		Texture2D,
		Texture3D,
		TextureCube
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

	inline std::string_view CmdListTypeToStr(ContextType type)
	{
		switch (type)
		{
		case ContextType::Direct:
			return "Direct";
		case ContextType::Copy:
			return "Copy";
		case ContextType::Compute:
			return "Compute";
		case ContextType::MAX:
		default:
			ensure(false);
			return "NONE";
		}
	}

	D3D12_COMMAND_LIST_TYPE D3DCmdListType(ContextType type);
	D3D12_RESOURCE_DIMENSION D3DTextureType(TextureType type);
	DXGI_FORMAT D3DFormat(Format format);

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

	inline uint16 CalculateMipCount(uint32 width, uint32 height = 0, uint32 depth = 0)
	{
		uint16 mipCount = 0;
		uint32 mipSize = Math::Max(width, Math::Max(height, depth));
		while (mipSize >= 1) { mipSize >>= 1; mipCount++; }
		return mipCount;
	}

	namespace Internal
	{
		void DXHandleError(HRESULT hr, const char* file, int line);
		void DXMessageCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext);
	}
}
#define DX_CHECK(expression) { HRESULT _hr = expression; if (_hr != S_OK) limbo::RHI::Internal::DXHandleError(_hr, __FILE__, __LINE__); }
