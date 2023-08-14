#include "stdafx.h"
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

	void ShaderBindingTable::BindRayGen(const char* name)
	{
		m_RayGenerationRecord = CreateShaderRecord(name);
	}

	void ShaderBindingTable::BindMissShader(const char* name)
	{
		m_MissShaderRecord = CreateShaderRecord(name);
	}

	void ShaderBindingTable::BindHitGroup(const char* name)
	{
		m_HitGroupRecord = CreateShaderRecord(name);
	}

	void ShaderBindingTable::Commit(D3D12_DISPATCH_RAYS_DESC& dispatchDesc, uint32 width, uint32 height, uint32 depth) const
	{
		Handle<Buffer> sbtBuffer = CreateBuffer({
			.DebugName = "ShaderBindingTable Buffer",
			.ByteSize = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT * 3, // we only support 1 of each shader
			.Usage = BufferUsage::Upload,
		});

		ID3D12Resource* sbtResource = ResourceManager::Ptr->GetBuffer(sbtBuffer)->Resource.Get();

		uint8* data;
		Map(sbtBuffer, (void**)&data);

		memcpy(data, m_RayGenerationRecord.Identifier, 32);
		data += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		memcpy(data, m_MissShaderRecord.Identifier, 32);
		data += D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;

		memcpy(data, m_HitGroupRecord.Identifier, 32);

		dispatchDesc = {
			.RayGenerationShaderRecord = sbtResource->GetGPUVirtualAddress(),
			.MissShaderTable = sbtResource->GetGPUVirtualAddress() + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
			.HitGroupTable = sbtResource->GetGPUVirtualAddress() + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT,
			.Width = width,
			.Height = height,
			.Depth = depth
		};

		Unmap(sbtBuffer);
		DestroyBuffer(sbtBuffer);
	}

	ShaderBindingTable::ShaderRecord ShaderBindingTable::CreateShaderRecord(const char* shaderIdentifier)
	{
		ShaderRecord result;

		ID3D12StateObjectProperties* properties;
		m_StateObject->QueryInterface(IID_PPV_ARGS(&properties));

		std::wstring widentifier;
		Utils::StringConvert(shaderIdentifier, widentifier);
		result.Identifier = properties->GetShaderIdentifier(widentifier.c_str());

		properties->Release();
		return result;
	}
}
