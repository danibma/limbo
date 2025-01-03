#include "stdafx.h"
#include "definitions.h"

#include <comdef.h>

namespace limbo::RHI
{
	namespace Internal
	{
		void DXHandleError(HRESULT hr, const char* file, int line)
		{
			_com_error err(hr);
			const char* errMsg = err.ErrorMessage();
			LB_ERROR("D3D12 Error: %s in %s:%d", errMsg, file, line);
		}

		void DXMessageCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
		{
			switch (Severity)
			{
			case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			case D3D12_MESSAGE_SEVERITY_ERROR:
				LB_ERROR("D3D12 GPU Validation: %s", pDescription);
				break;
			case D3D12_MESSAGE_SEVERITY_WARNING:
				LB_WARN("D3D12 GPU Validation: %s", pDescription);
				break;
			case D3D12_MESSAGE_SEVERITY_INFO:
			case D3D12_MESSAGE_SEVERITY_MESSAGE:
				LB_LOG("D3D12 GPU Validation: %s", pDescription);
				break;
			}
		}
	}

	#define FORMAT_TYPE(name) Format::name

	static constexpr FormatInfo sFormatInfo[] = {
	    // Format							Bytes	BlockSize  Components
	    { FORMAT_TYPE(UNKNOWN),				0,		0,		   0,		  },

	    // R
	    { FORMAT_TYPE(R8_UNORM),			1,		1,		   1,		  },
	    { FORMAT_TYPE(R8_UINT),				1,		1,		   1,		  },
	    { FORMAT_TYPE(R8_SINT),				1,		1,		   1,		  },
	    { FORMAT_TYPE(R8_SNORM),			1,		1,		   1,		  },
	    { FORMAT_TYPE(R16_UNORM),			2,		1,		   1,		  },
	    { FORMAT_TYPE(R16_SNORM),			2,		1,		   1,		  },
	    { FORMAT_TYPE(R16_UINT),			2,		1,		   1,		  },
	    { FORMAT_TYPE(R16_SINT),			2,		1,		   1,		  },
	    { FORMAT_TYPE(R16_FLOAT),			2,		1,		   1,		  },
	    { FORMAT_TYPE(R32_UINT),			4,		1,		   1,		  },
	    { FORMAT_TYPE(R32_SINT),			4,		1,		   1,		  },
	    { FORMAT_TYPE(R32_FLOAT),			4,		1,		   1,		  },

	    // RG
	    { FORMAT_TYPE(RG8_UINT),			2,		1,		   2,		  },
	    { FORMAT_TYPE(RG8_SINT),			2,		1,		   2,		  },
	    { FORMAT_TYPE(RG8_UNORM),			2,		1,		   2,		  },
	    { FORMAT_TYPE(RG8_SNORM),			2,		1,		   2,		  },
	    { FORMAT_TYPE(RG16_FLOAT),			4,		1,		   2,		  },
	    { FORMAT_TYPE(RG16_UINT),			4,		1,		   2,		  },
	    { FORMAT_TYPE(RG16_SINT),			4,		1,		   2,		  },
	    { FORMAT_TYPE(RG16_UNORM),			4,		1,		   2,		  },
	    { FORMAT_TYPE(RG16_SNORM),			4,		1,		   2,		  },
	    { FORMAT_TYPE(RG32_FLOAT),			8,		1,		   2,		  },
	    { FORMAT_TYPE(RG32_SINT),			8,		1,		   2,		  },
	    { FORMAT_TYPE(RG32_UINT),			8,		1,		   2,		  },

	    // RGB
	    { FORMAT_TYPE(RGB32_SINT),			12,		1,		   3,		  },
	    { FORMAT_TYPE(RGB32_UINT),			12,		1,		   3,		  },
	    { FORMAT_TYPE(RGB32_FLOAT),			12,		1,		   3,		  },

	    // RGBA
	    { FORMAT_TYPE(RGBA8_SNORM),			4,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA8_UNORM),			4,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA8_SINT),			4,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA8_UINT),			4,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA8_UNORM_SRGB),	4,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA16_UNORM),		8,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA16_SNORM),		8,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA16_FLOAT),		8,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA16_SINT),			8,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA16_UINT),			8,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA32_FLOAT),		16,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA32_SINT),			16,		1,		   4,		  },
	    { FORMAT_TYPE(RGBA32_UINT),			16,		1,		   4,		  },

	    // Depth
	    { FORMAT_TYPE(D16_UNORM),			2,		1,		   1,		  },
	    { FORMAT_TYPE(D32_FLOAT),			4,		1,		   1,		  },
	    { FORMAT_TYPE(D24_FLOAT_S8_UINT),	4,		1,		   1,		  },
	    { FORMAT_TYPE(D32_FLOAT_S8_UINT),	8,		1,		   1,		  },

	    // Compressed
	    { FORMAT_TYPE(BC1_UNORM),			8,		4,		   3,		  },
		{ FORMAT_TYPE(BC1_UNORM_SRGB),		8,		4,		   3,		  },
	    { FORMAT_TYPE(BC2_UNORM),			16,		4,		   4,		  },
	    { FORMAT_TYPE(BC2_UNORM_SRGB),		16,		4,		   4,		  },
	    { FORMAT_TYPE(BC3_UNORM),			16,		4,		   4,		  },
	    { FORMAT_TYPE(BC3_UNORM_SRGB),		16,		4,		   4,		  },
	    { FORMAT_TYPE(BC4_UNORM),			8,		4,		   1,		  },
	    { FORMAT_TYPE(BC4_SNORM),			8,		4,		   1,		  },
	    { FORMAT_TYPE(BC5_UNORM),			16,		4,		   2,		  },
	    { FORMAT_TYPE(BC5_SNORM),			16,		4,		   2,		  },
	    { FORMAT_TYPE(BC7_UNORM),			16,		4,		   4,		  },
		{ FORMAT_TYPE(BC7_UNORM_SRGB),		16,		4,		   4,		  },

	    { FORMAT_TYPE(R11G11B10_FLOAT),		4,		1,		   3,		  },
	    { FORMAT_TYPE(RGB10A2_UNORM),		4,		1,		   4,		  },
	    { FORMAT_TYPE(BGRA4_UNORM),			2,		1,		   4,		  },
	    { FORMAT_TYPE(B5G6R5_UNORM),		2,		1,		   3,		  },
	    { FORMAT_TYPE(B5G5R5A1_UNORM),		2,		1,		   4,		  },

	    { FORMAT_TYPE(BGRA8_UNORM),			4,		1,		   4,		  },
	};
	static_assert(ARRAYSIZE(sFormatInfo) == (uint32)Format::MAX);

	std::string_view CmdListTypeToStr(ContextType type)
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
	
	D3D12_COMMAND_LIST_TYPE D3DCmdListType(ContextType type)
	{
		switch (type)
		{
		case ContextType::Direct:
			return D3D12_COMMAND_LIST_TYPE_DIRECT;
		case ContextType::Copy:
			return D3D12_COMMAND_LIST_TYPE_COPY;
		case ContextType::Compute:
			return D3D12_COMMAND_LIST_TYPE_COMPUTE;
		case ContextType::MAX:
		default:
			ensure(false);
			return D3D12_COMMAND_LIST_TYPE_NONE;
		}
	}

	D3D12_RESOURCE_DIMENSION D3DTextureType(TextureType type)
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

	DXGI_FORMAT D3DFormat(Format format)
	{
		return gRHIPixelFormats[(int)format];
	}

	Format GetFormat(DXGI_FORMAT format)
	{
		for (int i = 0; i < (int)Format::MAX; ++i)
		{
			Format actualFormat = (Format)i;
			if (gRHIPixelFormats[i] == format)
				return actualFormat;
		}

		ensure(false);
		return Format::MAX;
	}

	bool IsFormatSRGB(Format format)
	{
		switch (format)
		{
		case Format::RGBA8_UNORM_SRGB: return true;
		case Format::BC1_UNORM_SRGB: return true;
		case Format::BC2_UNORM_SRGB: return true;
		case Format::BC3_UNORM_SRGB: return true;
		case Format::BC7_UNORM_SRGB: return true;
		}

		return false;
	}

	Format ConvertToSRGBFormat(Format format)
	{
		if (IsFormatSRGB(format)) return format;
		
		switch (format)
		{
		case Format::RGBA8_UNORM: return Format::RGBA8_UNORM_SRGB;
		case Format::BC1_UNORM: return Format::BC1_UNORM_SRGB;
		case Format::BC2_UNORM: return Format::BC2_UNORM_SRGB;
		case Format::BC3_UNORM: return Format::BC3_UNORM_SRGB;
		case Format::BC7_UNORM: return Format::BC7_UNORM_SRGB;
		default:
			ensure(false);
			return Format::UNKNOWN;
		}
	}

	uint16 CalculateMipCount(uint32 width, uint32 height, uint32 depth)
	{
		uint16 mipCount = 0;
		uint32 mipSize = Math::Max(width, Math::Max(height, depth));
		while (mipSize >= 1) { mipSize >>= 1; mipCount++; }
		return mipCount;
	}

	const FormatInfo& GetFormatInfo(Format format)
	{
		return sFormatInfo[(int)format];
	}

	uint64 GetRowPitch(Format format, uint32 width, uint32 mipIndex)
	{
		const FormatInfo& formatInfo = GetFormatInfo(format);
		if (formatInfo.BlockSize > 0)
		{
			uint64 numBlocks = Math::Max(1u, Math::DivideAndRoundUp(width >> mipIndex, (uint32)formatInfo.BlockSize));
			return numBlocks * formatInfo.BytesPerBlock;
		}
		return 0;
	}

	uint64 GetSlicePitch(Format format, uint32 width, uint32 height, uint32 mipIndex)
	{
		const FormatInfo& formatInfo = GetFormatInfo(format);
		if (formatInfo.BlockSize > 0)
		{
			uint64 numBlocksX = Math::Max(1u, Math::DivideAndRoundUp(width >> mipIndex, (uint32)formatInfo.BlockSize));
			uint64 numBlocksY = Math::Max(1u, Math::DivideAndRoundUp(height >> mipIndex, (uint32)formatInfo.BlockSize));
			return numBlocksX * numBlocksY * formatInfo.BytesPerBlock;
		}
		return 0;
	}

	uint64 GetTextureMipByteSize(Format format, uint32 width, uint32 height, uint32 depth, uint32 mipIndex)
	{
		return GetSlicePitch(format, width, height, mipIndex) * Math::Max(1u, depth >> mipIndex);
	}
}
