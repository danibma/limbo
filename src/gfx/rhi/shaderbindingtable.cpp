﻿#include "stdafx.h"
#include "shaderbindingtable.h"

#include "resourcemanager.h"
#include "shader.h"

namespace limbo::Gfx
{
	ShaderBindingTable::ShaderBindingTable(Handle<Shader> shader)
	{
		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		m_StateObject = s->StateObject.Get();
	}

	void ShaderBindingTable::BindRayGen(const wchar_t* name)
	{
		m_RayGenerationRecord = CreateShaderRecord(name);
	}

	void ShaderBindingTable::BindMissShader(const wchar_t* name)
	{
		m_MissShaderRecord = CreateShaderRecord(name);
	}

	void ShaderBindingTable::BindHitGroup(const wchar_t* name)
	{
		m_HitGroupRecord = CreateShaderRecord(name);
	}

	void ShaderBindingTable::Commit(D3D12_DISPATCH_RAYS_DESC& dispatchDesc, uint32 width, uint32 height, uint32 depth) const
	{
		Handle<Buffer> sbtBuffer = CreateBuffer({
			.DebugName = "ShaderBindingTable Buffer",
			.ByteSize = 256, // we only support 1 of each shader
			.Usage = BufferUsage::Upload,
		});

		ID3D12Resource* sbtResource = ResourceManager::Ptr->GetBuffer(sbtBuffer)->Resource.Get();

		Map(sbtBuffer);
		uint8* data = (uint8*)GetMappedData(sbtBuffer);

		memcpy(data, m_RayGenerationRecord.Identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		data += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		memcpy(data, m_MissShaderRecord.Identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		data += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		memcpy(data, m_HitGroupRecord.Identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		data += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		dispatchDesc = {
			.RayGenerationShaderRecord = {
				.StartAddress = sbtResource->GetGPUVirtualAddress(),
				.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			},
			.MissShaderTable = {
				.StartAddress = sbtResource->GetGPUVirtualAddress() + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
				.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
				.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			},
			.HitGroupTable = {
				.StartAddress = sbtResource->GetGPUVirtualAddress() + (D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * 2),
				.SizeInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
				.StrideInBytes = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
			},
			.Width = width,
			.Height = height,
			.Depth = depth
		};

		Unmap(sbtBuffer);
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