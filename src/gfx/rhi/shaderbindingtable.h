#pragma once
#include "resourcepool.h"

namespace limbo::RHI
{
	class Shader;
	class ShaderBindingTable
	{
		struct ShaderRecord
		{
			// note: at the moment we do not support local root signatures
			const void* Identifier = nullptr;
		};

	private:
		ID3D12StateObject*			m_StateObject;
		ShaderRecord				m_RayGenerationRecord;
		ShaderRecord				m_MissShaderRecord;
		ShaderRecord				m_HitGroupRecord;

	public:
		ShaderBindingTable() = default;
		ShaderBindingTable(Handle<Shader> shader);
		void BindRayGen(const wchar_t* name);
		void BindMissShader(const wchar_t* name);
		void BindHitGroup(const wchar_t* name);
		void Commit(D3D12_DISPATCH_RAYS_DESC& dispatchDesc, uint32 width, uint32 height, uint32 depth) const;

	private:
		ShaderRecord CreateShaderRecord(const wchar_t* shaderIdentifier);
	};
}

