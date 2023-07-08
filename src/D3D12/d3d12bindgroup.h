#pragma once

#include "shader.h"
#include "d3d12definitions.h"

namespace limbo::rhi
{
	class D3D12BindGroup : public BindGroup
	{
	private:
		struct RegisterCount
		{
			uint8 ShaderResourceCount;
			uint8 UnorderedAccessCount;
			uint8 ConstantBufferCount;
			uint8 SamplerCount;
		};
		RegisterCount m_registerCount{};

	public:
		ComPtr<ID3D12RootSignature>			rootSignature;

	public:
		D3D12BindGroup() = default;
		D3D12BindGroup(const BindGroupSpec& spec);
		virtual ~D3D12BindGroup();

	private:
		void initRegisterCount(const BindGroupSpec& spec);
	};
}

