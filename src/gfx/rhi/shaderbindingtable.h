#pragma once
#include "resourcepool.h"

namespace limbo::Gfx
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
		void BindRayGen(const char* name);
		void BindMissShader(const char* name);
		void BindHitGroup(const char* name);
		void Commit(D3D12_DISPATCH_RAYS_DESC& dispatchDesc, uint32 width, uint32 height, uint32 depth) const;

	private:
		ShaderRecord CreateShaderRecord(const char* shaderIdentifier);
	};
}

