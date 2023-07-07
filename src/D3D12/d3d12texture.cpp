#include "d3d12texture.h"

#include "d3d12device.h"

namespace limbo::rhi
{
	D3D12Texture::D3D12Texture(const TextureSpec& spec)
	{
		D3D12Device* device = Device::ptr->getAs<D3D12Device>();
		ID3D12Device* d3ddevice = device->getDevice();

		D3D12_RESOURCE_DESC desc = {
			.Dimension = d3dTextureType(spec.type),
			.Alignment = 0,
			.Width = spec.width,
			.Height = spec.height,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = d3dFormat(spec.format),
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		// #todo: use CreatePlacedResource instead of CreateCommittedResource
		D3D12_HEAP_PROPERTIES heapProps = {
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, 
													D3D12_RESOURCE_STATE_COMMON, 
													nullptr,
													IID_PPV_ARGS(&m_resource)));
	}

	D3D12Texture::~D3D12Texture()
	{
	}
}
