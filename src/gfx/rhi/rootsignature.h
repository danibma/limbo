#pragma once

namespace limbo::RHI
{
	class RootSignature
	{
	private:
		struct RootParameter
		{
			CD3DX12_ROOT_PARAMETER1		Parameter;
			CD3DX12_DESCRIPTOR_RANGE1	Range;
		};

		static constexpr uint8 ROOT_PARAMETER_NUM = 8;

	private:
		using TRootParameterMap = std::array<RootParameter, ROOT_PARAMETER_NUM>;

		std::string							m_Name;
		ComPtr<ID3D12RootSignature>			m_RS;
		TRootParameterMap					m_RootParameters;
		uint32								m_NumRootParameters;
		std::vector<uint32>					m_DescriptorTablesDescriptorAmount;

	public:
		RootSignature(const std::string& name);
		~RootSignature() {}

		template<typename T>
		void AddRootConstants(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL)
		{
			AddRootConstants(shaderRegister, sizeof(T) / sizeof(uint32), space, visibility);
		}
		void AddRootConstants(uint32 shaderRegister, uint32 constantCount, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void AddRootCBV(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void AddRootSRV(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void AddRootUAV(uint32 shaderRegister, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);
		void AddDescriptorTable(uint32 shaderRegister, uint32 numDescriptors, D3D12_DESCRIPTOR_RANGE_TYPE type, uint32 space = 0, D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL);

		void Create(D3D12_ROOT_SIGNATURE_FLAGS flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

		const std::vector<uint32>& GetDescriptorTablesDescriptorAmount() const { return m_DescriptorTablesDescriptorAmount; }

		ID3D12RootSignature* Get() const { return m_RS.Get(); }
		void Reset();

	private:
		uint32 GetDWORDCost();
	};

}
