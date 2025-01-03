﻿#include "stdafx.h"
#include "shaderbindingtable.h"
#include "resourcemanager.h"
#include "shader.h"
#include "pipelinestateobject.h"

namespace limbo::RHI
{
	ShaderBindingTable::ShaderBindingTable(PSOHandle pso)
	{
		m_StateObject = RM_GET(pso)->GetStateObject();
	}

	void ShaderBindingTable::BindRayGen(const wchar_t* name)
	{
		m_RayGenerationRecord = CreateShaderRecord(name);
	}

	void ShaderBindingTable::AddMissShader(const wchar_t* name)
	{
		m_MissShaderRecords.emplace_back(CreateShaderRecord(name));
	}

	void ShaderBindingTable::AddHitGroup(const wchar_t* name)
	{
		m_HitGroupRecords.emplace_back(CreateShaderRecord(name));
	}

	void ShaderBindingTable::Commit(D3D12_DISPATCH_RAYS_DESC& dispatchDesc, uint32 width, uint32 height, uint32 depth) const
	{
		BufferHandle sbtBuffer = CreateBuffer({
			.DebugName = "ShaderBindingTable Buffer",
			.ByteSize = 256, // we only support 1 of each shader
			.Flags = BufferUsage::Upload,
		});

		RHI::Buffer* pSbtBuffer = RM_GET(sbtBuffer);
		ID3D12Resource* sbtResource = pSbtBuffer->Resource.Get();

		pSbtBuffer->Map();
		uint8* data = (uint8*)pSbtBuffer->MappedData;

		memcpy(data, m_RayGenerationRecord.Identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		data += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		for (const ShaderRecord& record : m_MissShaderRecords)
		{
			memcpy(data, record.Identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			data += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
		}
		data += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		for (const ShaderRecord& record : m_HitGroupRecords)
		{
			memcpy(data, record.Identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			data += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;	
		}
		data += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		dispatchDesc = {
			.RayGenerationShaderRecord = {
				.StartAddress = sbtResource->GetGPUVirtualAddress(),
				.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			},
			.MissShaderTable = {
				.StartAddress = sbtResource->GetGPUVirtualAddress() + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
				.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * m_MissShaderRecords.size(),
				.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			},
			.HitGroupTable = {
				.StartAddress = sbtResource->GetGPUVirtualAddress() + (D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * (m_MissShaderRecords.size() + 1)),
				.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES * m_HitGroupRecords.size(),
				.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			},
			.Width = width,
			.Height = height,
			.Depth = depth
		};

		pSbtBuffer->Unmap();
		DestroyBuffer(sbtBuffer);
	}

	ShaderBindingTable::ShaderRecord ShaderBindingTable::CreateShaderRecord(const wchar_t* shaderIdentifier)
	{
		ShaderRecord result;

		ID3D12StateObjectProperties* properties;
		m_StateObject->QueryInterface(IID_PPV_ARGS(&properties));

		result.Identifier = properties->GetShaderIdentifier(shaderIdentifier);

		properties->Release();
		return result;
	}
}
