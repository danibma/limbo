#pragma once
#include "pipelinestateobject.h"

namespace limbo::RHI
{
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
		std::vector<ShaderRecord>	m_MissShaderRecords;
		std::vector<ShaderRecord>	m_HitGroupRecords;

	public:
		ShaderBindingTable() = default;
		ShaderBindingTable(PSOHandle pso);
		void BindRayGen(const wchar_t* name);
		void AddMissShader(const wchar_t* name);
		void AddHitGroup(const wchar_t* name);
		void Commit(D3D12_DISPATCH_RAYS_DESC& dispatchDesc, uint32 width, uint32 height, uint32 depth) const;

	private:
		ShaderRecord CreateShaderRecord(const wchar_t* shaderIdentifier);
	};
}

