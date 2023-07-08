#include "d3d12bindgroup.h"
#include "d3d12device.h"
#include "d3d12resourcemanager.h"

#include <d3d12/d3dx12/d3dx12.h>


namespace limbo::rhi
{
	void D3D12BindGroup::initRegisterCount(const BindGroupSpec& spec)
	{
		D3D12ResourceManager* rm = ResourceManager::getAs<D3D12ResourceManager>();

		for (auto [slot, buffer] : spec.buffers)
		{
			ensure(false);
		}

		for (auto [slot, texture] : spec.textures)
		{
			D3D12Texture* d3dTexture = rm->getTexture(texture);
			if (d3dTexture->isUnordered())
				m_registerCount.UnorderedAccessCount++;
			else
				m_registerCount.ShaderResourceCount++;
		}
	}

	D3D12BindGroup::D3D12BindGroup(const BindGroupSpec& spec)
	{
		D3D12Device* device = Device::getAs<D3D12Device>();
		ID3D12Device* d3ddevice = device->getDevice();

		initRegisterCount(spec);

		uint8 rootParamterCount = 0;
		CD3DX12_ROOT_PARAMETER1 rootParameters[32];

		if (m_registerCount.ShaderResourceCount > 0)
		{
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, m_registerCount.ShaderResourceCount, 0);
			rootParameters[rootParamterCount].InitAsDescriptorTable(1, &range);
			rootParamterCount++;
		}

		if (m_registerCount.ConstantBufferCount > 0)
		{
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, m_registerCount.ConstantBufferCount, 0);
			rootParameters[rootParamterCount].InitAsDescriptorTable(1, &range);
			rootParamterCount++;
		}

		if (m_registerCount.UnorderedAccessCount > 0)
		{
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, m_registerCount.UnorderedAccessCount, 0);
			rootParameters[rootParamterCount].InitAsDescriptorTable(1, &range);
			rootParamterCount++;
		}

		if (m_registerCount.SamplerCount > 0)
		{
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, m_registerCount.SamplerCount, 0);
			rootParameters[rootParamterCount].InitAsDescriptorTable(1, &range);
			rootParamterCount++;
		}

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc(rootParamterCount, rootParameters);
		ID3DBlob* rootSigBlob;
		ID3DBlob* errorBlob;
		DX_CHECK(D3D12SerializeVersionedRootSignature(&desc, &rootSigBlob, &errorBlob));
		if (errorBlob)
		{
			LB_ERROR("%s", (char*)errorBlob->GetBufferPointer());
		}

		DX_CHECK(d3ddevice->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	}

	D3D12BindGroup::~D3D12BindGroup()
	{
	}
}
