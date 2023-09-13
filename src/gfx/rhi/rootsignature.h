#pragma once

namespace limbo::RHI
{
	namespace SC { struct Kernel; }

	struct ShaderParameterInfo
	{
		ShaderParameterType	Type;
		uint32				RPIndex;

		union
		{
			// 16 bytes
			struct
			{
				const void* Data;
				uint32		 NumValues; // Num32BitValuesToSet
				uint32		 Offset;    // DestOffsetIn32BitValues
			};

			// 8 bytes
			struct
			{
				D3D12_GPU_DESCRIPTOR_HANDLE Descriptor;
			};
		};

		bool IsValid() { return Type != ShaderParameterType::MAX; }
	};

	class RootSignature
	{
	private:
		using TParameterMap = std::unordered_map<uint32, ShaderParameterInfo>;

		std::string							m_Name;
		ComPtr<ID3D12RootSignature>			m_RS;
		TParameterMap						m_ParameterMap;

	public:
		RootSignature(const std::string& name);
		~RootSignature() {}

		void CreateRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags, SC::Kernel* kernels, uint32 kernelsCount);

		void SetRootParameters(ShaderType shaderType, ID3D12GraphicsCommandList* cmd);
		ShaderParameterInfo& GetParameter(const char* parameterName);
		ID3D12RootSignature* Get() const { return m_RS.Get(); }
		void Reset();
	};

}
