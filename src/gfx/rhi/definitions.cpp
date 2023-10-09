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

}
