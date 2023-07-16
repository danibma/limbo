#include "bindgroup.h"
#include "device.h"
#include "resourcemanager.h"

#include <d3d12/d3dx12/d3dx12.h>

namespace limbo::gfx
{
	void BindGroup::initRegisterCount(const BindGroupSpec& spec)
	{
		ResourceManager* rm = ResourceManager::ptr;

		for (auto [slot, buffer] : spec.buffers)
		{
			ensure(false);
		}

		for (auto [slot, texture] : spec.textures)
		{
			Texture* d3dTexture = rm->getTexture(texture);
			if (d3dTexture->bIsUnordered)
			{
				m_registerCount.UnorderedAccessCount++;
				m_tables[(uint8)TableType::UAV].handles.emplace_back(d3dTexture->handle.cpuHandle);
				uavTextures.emplace_back(d3dTexture);
			}
			else
			{
				m_registerCount.ShaderResourceCount++;
				ensure(false);
			}
		}
	}

	BindGroup::BindGroup(const BindGroupSpec& spec)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		m_tables[(uint8)TableType::SRV].begin			= SRV_BEGIN;
		m_tables[(uint8)TableType::CBV].begin			= CBV_BEGIN;
		m_tables[(uint8)TableType::UAV].begin			= UAV_BEGIN;
		m_tables[(uint8)TableType::SAMPLERS].begin		= SAMPLERS_BEGIN;

		m_localHeap = new DescriptorHeap(d3ddevice, DescriptorHeapType::SRV, true);

		initRegisterCount(spec);

		uint8 rootParamterCount = 0;
		CD3DX12_ROOT_PARAMETER1 rootParameters[32];

		if (m_registerCount.ShaderResourceCount > 0)
		{
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, m_registerCount.ShaderResourceCount, 0);
			rootParameters[rootParamterCount].InitAsDescriptorTable(1, &range);
			m_tables[(uint8)TableType::SRV].index = rootParamterCount;
			rootParamterCount++;
		}

		if (m_registerCount.ConstantBufferCount > 0)
		{
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, m_registerCount.ConstantBufferCount, 0);
			rootParameters[rootParamterCount].InitAsDescriptorTable(1, &range);
			m_tables[(uint8)TableType::CBV].index = rootParamterCount;
			rootParamterCount++;
		}

		if (m_registerCount.UnorderedAccessCount > 0)
		{
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, m_registerCount.UnorderedAccessCount, 0);
			rootParameters[rootParamterCount].InitAsDescriptorTable(1, &range);
			m_tables[(uint8)TableType::UAV].index = rootParamterCount;
			rootParamterCount++;
		}

		if (m_registerCount.SamplerCount > 0)
		{
			CD3DX12_DESCRIPTOR_RANGE1 range;
			range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, m_registerCount.SamplerCount, 0);
			rootParameters[rootParamterCount].InitAsDescriptorTable(1, &range);
			m_tables[(uint8)TableType::SAMPLERS].index = rootParamterCount;
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

		for (uint8 i = 0; i < (uint8)TableType::MAX; ++i)
			copyDescriptors((TableType)i);
	}

	BindGroup::~BindGroup()
	{
		delete m_localHeap;
	}

	void BindGroup::setComputeRootParameters(ID3D12GraphicsCommandList* cmd)
	{
		transitionResources();

		ID3D12DescriptorHeap* heaps[] = { m_localHeap->getHeap() };
		cmd->SetDescriptorHeaps(1, heaps);

		for (uint8 i = 0; i < (uint8)TableType::MAX; ++i)
		{
			const Table& table = m_tables[i];
			if (table.index == -1) continue;

			cmd->SetComputeRootDescriptorTable(table.index, m_localHeap->getHandleByIndex(table.begin).gpuHandle);
		}
	}

	void BindGroup::setGraphicsRootParameters(ID3D12GraphicsCommandList* cmd)
	{
		ensure(false);
	}

	void BindGroup::transitionResources()
	{
		Device* device = Device::ptr;

		for (Texture* t : uavTextures)
			device->transitionResource(t, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		for (Texture* t : srvTextures)
		{
			ensure(false);
			device->transitionResource(t, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		}
	}

	void BindGroup::copyDescriptors(TableType type)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		const Table& table = m_tables[(uint8)type];
		if (table.index == -1) return;

		uint32 descriptorsCount = (uint32)table.handles.size();

		D3D12_CPU_DESCRIPTOR_HANDLE start = m_localHeap->getHandleByIndex(table.begin).cpuHandle;
		d3ddevice->CopyDescriptors(1, &start, &descriptorsCount, 1, table.handles.data(), &descriptorsCount, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}
