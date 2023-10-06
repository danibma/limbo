#include "stdafx.h"
#include "rootsignature.h"

#include "device.h"
#include "shadercompiler.h"

namespace limbo::RHI
{
	static D3D12_DESCRIPTOR_RANGE_FLAGS SDefaultTableRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE | D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
	static D3D12_ROOT_DESCRIPTOR_FLAGS SDefaultRootDescriptorFlags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;

	RootSignature::RootSignature(const std::string& name)
		: m_Name(name), m_NumRootParameters(0)
	{
	}

	void RootSignature::AddRootConstants(uint32 shaderRegister, uint32 constantCount, uint32 space, D3D12_SHADER_VISIBILITY visibility)
	{
		RootParameter& parameter = m_RootParameters[m_NumRootParameters++];
		parameter.Parameter.InitAsConstants(constantCount, shaderRegister, space, visibility);
		m_DescriptorTablesDescriptorAmount.emplace_back(0);
	}

	void RootSignature::AddRootCBV(uint32 shaderRegister, uint32 space, D3D12_SHADER_VISIBILITY visibility)
	{
		RootParameter& parameter = m_RootParameters[m_NumRootParameters++];
		parameter.Parameter.InitAsConstantBufferView(shaderRegister, space, SDefaultRootDescriptorFlags, visibility);
		m_DescriptorTablesDescriptorAmount.emplace_back(0);
	}

	void RootSignature::AddRootSRV(uint32 shaderRegister, uint32 space, D3D12_SHADER_VISIBILITY visibility)
	{
		RootParameter& parameter = m_RootParameters[m_NumRootParameters++];
		parameter.Parameter.InitAsShaderResourceView(shaderRegister, space, SDefaultRootDescriptorFlags, visibility);
		m_DescriptorTablesDescriptorAmount.emplace_back(0);
	}

	void RootSignature::AddRootUAV(uint32 shaderRegister, uint32 space, D3D12_SHADER_VISIBILITY visibility)
	{
		RootParameter& parameter = m_RootParameters[m_NumRootParameters++];
		parameter.Parameter.InitAsUnorderedAccessView(shaderRegister, space, SDefaultRootDescriptorFlags, visibility);
		m_DescriptorTablesDescriptorAmount.emplace_back(0);
	}

	void RootSignature::AddDescriptorTable(uint32 shaderRegister, uint32 numDescriptors, D3D12_DESCRIPTOR_RANGE_TYPE type, uint32 space, D3D12_SHADER_VISIBILITY visibility)
	{
		RootParameter& parameter = m_RootParameters[m_NumRootParameters++];
		parameter.Range.Init(type, numDescriptors, shaderRegister, space, SDefaultTableRangeFlags, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
		parameter.Parameter.InitAsDescriptorTable(1, &parameter.Range, visibility);
		m_DescriptorTablesDescriptorAmount.emplace_back(numDescriptors);
	}

	void RootSignature::Create(D3D12_ROOT_SIGNATURE_FLAGS flags)
	{
		D3D12_STATIC_SAMPLER_DESC staticSamplers[16] = {};
		auto addStaticSampler = [&staticSamplers](uint32 registerSlot, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE wrapMode, D3D12_COMPARISON_FUNC compareFunc = D3D12_COMPARISON_FUNC_ALWAYS)
		{
			staticSamplers[registerSlot] = {
				.Filter = filter,
				.AddressU = wrapMode,
				.AddressV = wrapMode,
				.AddressW = wrapMode,
				.MipLODBias = 0.0f,
				.MaxAnisotropy = 8,
				.ComparisonFunc = compareFunc,
				.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
				.MinLOD = 0.0f,
				.MaxLOD = FLT_MAX,
				.ShaderRegister = registerSlot,
				.RegisterSpace = 1,
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
			};
		};

		flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		// Static Samplers: these are the equivalents of the samplers in bindings.hlsl
		uint32 numStaticSamplers = 0;
		addStaticSampler(numStaticSamplers++, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);   // SLinearWrap
		addStaticSampler(numStaticSamplers++, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // SLinearClamp
		addStaticSampler(numStaticSamplers++, D3D12_FILTER_MIN_MAG_MIP_POINT,  D3D12_TEXTURE_ADDRESS_MODE_WRAP);   // SPointWrap

		constexpr uint32 recommendedDwords = 12;
		uint32 rsCost = GetDWORDCost();
		ensure(rsCost < 64);
		if (rsCost > recommendedDwords)
			LB_LOG("RootSignature '%s' uses %d DWORDs while under %d is recommended", m_Name.c_str(), rsCost, recommendedDwords);

		CD3DX12_ROOT_PARAMETER1 rootParameters[ROOT_PARAMETER_NUM];
		for (uint32 i = 0; i < m_NumRootParameters; ++i)
			rootParameters[i] = m_RootParameters[i].Parameter;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Init_1_1(m_NumRootParameters, rootParameters, numStaticSamplers, staticSamplers, flags);

		ID3DBlob* rootSigBlob;
		ID3DBlob* errorBlob;
		D3D12SerializeVersionedRootSignature(&desc, &rootSigBlob, &errorBlob);
		if (errorBlob)
			LB_ERROR("%s", (char*)errorBlob->GetBufferPointer());

		DX_CHECK(Device::Ptr->GetDevice()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(m_RS.ReleaseAndGetAddressOf())));

		std::wstring wname;
		Utils::StringConvert(m_Name, wname);
		DX_CHECK(m_RS->SetName(wname.c_str()));
	}

	void RootSignature::Reset()
	{
		m_RS.Reset();
	}

	uint32 RootSignature::GetDWORDCost()
	{
		uint32 cost = 0;

		for (uint32 i = 0; i < m_NumRootParameters; ++i)
		{
			const RootParameter& rootParameter = m_RootParameters[i];
			switch (rootParameter.Parameter.ParameterType)
			{
			case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
				cost += rootParameter.Parameter.Constants.Num32BitValues;
				break;
			case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
				cost += 1;
				break;
			case D3D12_ROOT_PARAMETER_TYPE_CBV:
			case D3D12_ROOT_PARAMETER_TYPE_SRV:
			case D3D12_ROOT_PARAMETER_TYPE_UAV:
				cost += 2;
				break;
			}
		}

		return cost;
	}
}
