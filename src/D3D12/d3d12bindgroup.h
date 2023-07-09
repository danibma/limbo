#pragma once

#include "shader.h"
#include "d3d12definitions.h"
#include "d3d12descriptorheap.h"

namespace limbo::rhi
{
	class D3D12Buffer;
	class D3D12Texture;
	constexpr uint8 SRV_MAX			= 32;
	constexpr uint8 UAV_MAX			= 32;
	constexpr uint8 CBV_MAX			= 32;
	constexpr uint8 SAMPLERS_MAX	= 16;

	constexpr uint8 SRV_BEGIN			= 0;
	constexpr uint8 UAV_BEGIN			= SRV_BEGIN + SRV_MAX   + 1;
	constexpr uint8 CBV_BEGIN			= UAV_BEGIN + UAV_BEGIN + 1;
	constexpr uint8 SAMPLERS_BEGIN		= CBV_BEGIN + CBV_MAX   + 1;

	class D3D12BindGroup : public BindGroup
	{
	private:
		struct RegisterCount
		{
			uint8 ShaderResourceCount   = 0;
			uint8 UnorderedAccessCount  = 0;
			uint8 ConstantBufferCount   = 0;
			uint8 SamplerCount			= 0;
		};
		RegisterCount m_registerCount{};

		enum class TableType : uint8
		{
			SRV = 0,
			UAV,
			CBV,
			SAMPLERS,
			MAX
		};

		struct Table
		{
			int							index = -1;
			uint8						begin =  0;

			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles;
		};
		Table m_tables[(uint8)TableType::MAX];

		D3D12DescriptorHeap* m_localHeap;

		// resource lists
		std::vector<D3D12Texture*> uavTextures;
		std::vector<D3D12Texture*> srvTextures;

	public:
		ComPtr<ID3D12RootSignature>			rootSignature;

	public:
		D3D12BindGroup() = default;
		D3D12BindGroup(const BindGroupSpec& spec);
		virtual ~D3D12BindGroup();

		// D3D12 specific
		void setComputeRootParameters(ID3D12GraphicsCommandList* cmd);
		void setGraphicsRootParameters(ID3D12GraphicsCommandList* cmd);
		void transitionResources();

	private:
		void initRegisterCount(const BindGroupSpec& spec);
		void copyDescriptors(TableType type);
	};
}

