#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "shadercompiler.h"

#include <unordered_map>

struct CD3DX12_ROOT_PARAMETER1;

namespace limbo::gfx
{
	struct ShaderSpec
	{
		const char* programName;
		const char* vs_entryPoint = "VSMain";
		const char* ps_entryPoint = "PSMain";
		const char* cs_entryPoint = "CSMain";

		uint8  rtCount;
		Format rtFormats[8];
		Format depthFormat = Format::MAX;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE	topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		ShaderType						type = ShaderType::Graphics;
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
			ParameterType	type;
			uint32			rpIndex;

			union
			{
				// 16 bytes
				struct
				{
					const void*  data;
					uint32		 numValues; // Num32BitValuesToSet
					uint32		 offset; // DestOffsetIn32BitValues
				};

				// 8 bytes
				struct
				{
					D3D12_GPU_DESCRIPTOR_HANDLE descriptor;
				};
			};
		};
		using ParameterMap = std::unordered_map<uint32, ParameterInfo>;
		using InputLayout = std::vector<D3D12_INPUT_ELEMENT_DESC>;

	public:
		ComPtr<ID3D12PipelineState>			pipelineState;
		ComPtr<ID3D12RootSignature>			rootSignature;
		ParameterMap						parameterMap;
		ShaderType							type;
		bool								useSwapchainRT;

	public:
		Shader() = default;
		Shader(const ShaderSpec& spec);

		~Shader();

		void setComputeRootParameters(ID3D12GraphicsCommandList* cmd);
		void setGraphicsRootParameters(ID3D12GraphicsCommandList* cmd);

		void setConstant(const char* parameterName, const void* data);
		void setTexture(const char* parameterName, Handle<class Texture> texture);
		void setBuffer(const char* parameterName, Handle<class Buffer> buffer);
		void setSampler(const char* parameterName, Handle<class Sampler> sampler);

	private:
		void createInputLayout(const SC::Kernel& vs, InputLayout& outInputLayout);
		void createRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags, const SC::Kernel* kernels, uint32 kernelsCount);
		void createComputePipeline(ID3D12Device* device, const ShaderSpec& spec);
		void createGraphicsPipeline(ID3D12Device* device, const ShaderSpec& spec);
	};
}
