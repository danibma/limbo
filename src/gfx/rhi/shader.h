#pragma once

#include "definitions.h"
#include "resourcepool.h"

#include <unordered_map>

struct CD3DX12_ROOT_PARAMETER1;

namespace limbo::Gfx::SC
{
	struct Kernel;
}

namespace limbo::Gfx
{
	struct ShaderSpec
	{
	public:
		struct RenderTarget
		{
			Format			RTFormat;
			const char*		DebugName = "";
		};

	public:
		const char*						ProgramName;
		const char*						VSEntryPoint = "VSMain";
		const char*						PSEntryPoint = "PSMain";
		const char*						CsEntryPoint = "CSMain";

		RenderTarget					RTFormats[8];
		RenderTarget					DepthFormat = { Format::UNKNOWN };

		D3D12_PRIMITIVE_TOPOLOGY_TYPE	Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		ShaderType						Type = ShaderType::Graphics;
	};

	class Shader
	{
		enum class ParameterType : uint8
		{
			SRV = 0,
			UAV,
			Samplers,
			Constants,
			MAX
		};

		struct ParameterInfo
		{
			ParameterType	Type;
			uint32			RPIndex;

			union
			{
				// 16 bytes
				struct
				{
					const void*  Data;
					uint32		 NumValues; // Num32BitValuesToSet
					uint32		 Offset;    // DestOffsetIn32BitValues
				};

				// 8 bytes
				struct
				{
					D3D12_GPU_DESCRIPTOR_HANDLE Descriptor;
				};
			};
		};
		using TParameterMap = std::unordered_map<uint32, ParameterInfo>;
		using InputLayout = std::vector<D3D12_INPUT_ELEMENT_DESC>;

	public:
		ComPtr<ID3D12PipelineState>			PipelineState;
		ComPtr<ID3D12RootSignature>			RootSignature;
		TParameterMap						ParameterMap;
		ShaderType							Type;
		bool								UseSwapchainRT;

		uint8								RTCount;
		Handle<class Texture>				RenderTargets[8];
		Handle<class Texture>				DepthTarget;

	public:
		Shader() = default;
		Shader(const ShaderSpec& spec);

		~Shader();

		void SetComputeRootParameters(ID3D12GraphicsCommandList* cmd);
		void SetGraphicsRootParameters(ID3D12GraphicsCommandList* cmd);

		void SetConstant(const char* parameterName, const void* data);
		void SetTexture(const char* parameterName, Handle<class Texture> texture);
		void SetBuffer(const char* parameterName, Handle<class Buffer> buffer);
		void SetSampler(const char* parameterName, Handle<class Sampler> sampler);

	private:
		void CreateInputLayout(const SC::Kernel& vs, InputLayout& outInputLayout);
		void CreateRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags, const SC::Kernel* kernels, uint32 kernelsCount);
		void CreateComputePipeline(ID3D12Device* device, const ShaderSpec& spec);
		void CreateGraphicsPipeline(ID3D12Device* device, const ShaderSpec& spec);
	};
}
