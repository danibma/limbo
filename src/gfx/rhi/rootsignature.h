#pragma once

#include "core/refcountptr.h"
#include "resourcepool.h"
#include "core/array.h"

namespace limbo::RHI
{
	inline constexpr uint8 ROOT_PARAMETER_NUM = 8;

	struct RSSpec
	{
	private:
		struct RootParameter
		{
			CD3DX12_ROOT_PARAMETER1		Parameter;
			CD3DX12_DESCRIPTOR_RANGE1	Range;
		};

		using TRootParameterMap = TStaticArray<RootParameter, ROOT_PARAMETER_NUM>;

	public:
		TRootParameterMap			RootParameters;
		uint32						NumRootParameters;
		std::vector<uint32>			DescriptorTablesDescriptorAmount;
		D3D12_ROOT_SIGNATURE_FLAGS  Flags;

		template<typename T>
		RSSpec& AddRootConstants(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			return AddRootConstants(shaderRegister, sizeof(T) / sizeof(uint32), space, visibility);
		}
		RSSpec& Init();
		RSSpec& AddRootConstants(uint32 shaderRegister, uint32 constantCount, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		RSSpec& AddRootCBV(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		RSSpec& AddRootSRV(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		RSSpec& AddRootUAV(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		RSSpec& AddDescriptorTable(uint32 shaderRegister, uint32 numDescriptors, D3D12_DESCRIPTOR_RANGE_TYPE type, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		RSSpec& SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);
	};

	class RootSignature
	{
	private:
		std::string							m_Name;
		RefCountPtr<ID3D12RootSignature>	m_RS;

	public:
		RootSignature() = default;
		RootSignature(const std::string& name, const RSSpec& initializer);
		~RootSignature() {}

		ID3D12RootSignature* Get() const { return m_RS.Get(); }
		void Reset();

	private:
		uint32 GetDWORDCost(const RSSpec& initializer);
	};
	typedef Handle<RootSignature> RootSignatureHandle;
}
