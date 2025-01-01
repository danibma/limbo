#pragma once

#include "core/core.h"

namespace limbo::RHI
{
	namespace Delegates
	{
		DECLARE_MULTICAST_DELEGATE(TOnPrepareFrame);
		inline TOnPrepareFrame OnPrepareFrame;

		DECLARE_MULTICAST_DELEGATE(TOnResizedSwapchain, uint32, uint32);
		inline TOnResizedSwapchain OnResizedSwapchain;

		DECLARE_MULTICAST_DELEGATE(TOnShadersReloaded);
		inline TOnShadersReloaded OnShadersReloaded;
	}

	namespace Internal
	{
		void DXHandleError(HRESULT hr, const char* file, int line);
		void DXMessageCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext);
	}

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
		UNKNOWN = 0,

		// R
		R8_UNORM,
		R8_UINT,
		R8_SINT,
		R8_SNORM,
		R16_UNORM,
		R16_SNORM,
		R16_UINT,
		R16_SINT,
		R16_FLOAT,
		R32_UINT,
		R32_SINT,
		R32_FLOAT,
		
		// RG
		RG8_UINT,
		RG8_SINT,
		RG8_UNORM,
		RG8_SNORM,
		RG16_FLOAT,
		RG16_UINT,
		RG16_SINT,
		RG16_UNORM,
		RG16_SNORM,
		RG32_FLOAT,
		RG32_SINT,
		RG32_UINT,
		
		// RGB
		RGB32_SINT,
		RGB32_UINT,
		RGB32_FLOAT,
		
		// RGBA
		RGBA8_SNORM,
		RGBA8_UNORM,
		RGBA8_SINT,
		RGBA8_UINT,
		RGBA8_UNORM_SRGB,
		RGBA16_UNORM,
		RGBA16_SNORM,
		RGBA16_FLOAT,
		RGBA16_SINT,
		RGBA16_UINT,
		RGBA32_FLOAT,
		RGBA32_SINT,
		RGBA32_UINT,
		
		// DEPTH
		D16_UNORM,
		D32_FLOAT,
		D24_FLOAT_S8_UINT,
		D32_FLOAT_S8_UINT,
		
		// Compressed
		BC1_UNORM,
		BC2_UNORM,
		BC3_UNORM,
		BC4_UNORM,
		BC4_SNORM,
		BC5_UNORM,
		BC5_SNORM,
		BC7_UNORM,

		R11G11B10_FLOAT,
		RGB10A2_UNORM,
		BGRA4_UNORM,
		B5G6R5_UNORM,
		B5G5R5A1_UNORM,
		
		//Surface
		BGRA8_UNORM,
		
		MAX
	};

	struct FormatInfo
	{
		Format			Format;
		uint8			BytesPerBlock : 8;
		uint8			BlockSize : 4;
		uint8			NumComponents : 3;
	};

	std::string_view CmdListTypeToStr(ContextType type);

	D3D12_COMMAND_LIST_TYPE D3DCmdListType(ContextType type);
	D3D12_RESOURCE_DIMENSION D3DTextureType(TextureType type);
	DXGI_FORMAT D3DFormat(Format format);

	Format GetFormat(DXGI_FORMAT format);

	uint16 CalculateMipCount(uint32 width, uint32 height = 0, uint32 depth = 0);
	const FormatInfo& GetFormatInfo(Format format);
	uint64 GetRowPitch(Format format, uint32 width, uint32 mipIndex = 0);
	uint64 GetSlicePitch(Format format, uint32 width, uint32 height, uint32 mipIndex = 0);
	uint64 GetTextureMipByteSize(Format format, uint32 width, uint32 height, uint32 depth, uint32 mipIndex = 0);

	inline DXGI_FORMAT gRHIPixelFormats[(uint32)Format::MAX];

	constexpr uint8 gRHIBufferCount = 3;
	constexpr uint8 gRHIMaxRenderTargets = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
}
#define DX_CHECK(expression) { HRESULT _hr = expression; if (_hr != S_OK) limbo::RHI::Internal::DXHandleError(_hr, __FILE__, __LINE__); }
