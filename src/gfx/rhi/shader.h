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
		using HitGroupDescList = std::vector<D3D12_HIT_GROUP_DESC>;

	public:
		struct RenderTargetDesc
		{
			Format					RTFormat;
			const char*				DebugName = ""; // If the RTTexture is set, this does nothing

			Handle<class Texture>	RTTexture;

			RenderPassOp			LoadRenderPassOp = RenderPassOp::Clear;
		};

	public:
		const char*						ProgramName;
		const char*						VSEntryPoint = "VSMain";
		const char*						PSEntryPoint = "PSMain";
		const char*						CsEntryPoint = "CSMain";

		RenderTargetDesc				RTFormats[8];
		RenderTargetDesc				DepthFormat = { Format::UNKNOWN };

		D3D12_PRIMITIVE_TOPOLOGY_TYPE	Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// Raytracing stuff
		HitGroupDescList				HitGroupsDescriptions;
		std::vector<D3D12_EXPORT_DESC>	Exports;

		ShaderType						Type = ShaderType::Graphics;
	};

	class Shader
	{
		enum class ParameterType : uint8
		{
			SRV = 0,
			UAV,
			CBV,
			Samplers,
			Constants,
			MAX
		};

		struct RenderTarget
		{
			Handle<class Texture>	Texture;
			RenderPassOp			LoadRenderPassOp;
			bool					bIsExternal = false; // if the render target comes from another shader, this is true
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

		std::string							m_Name;
		ShaderSpec							m_Spec;
		DelegateHandle						m_ReloadShaderDelHandle;

	public:
		ComPtr<ID3D12PipelineState>			PipelineState;
		ComPtr<ID3D12StateObject>			StateObject; // for RayTracing
		ComPtr<ID3D12RootSignature>			RootSignature;
		TParameterMap						ParameterMap;
		ShaderType							Type;
		bool								UseSwapchainRT;

		uint8								RTCount;
		RenderTarget						RenderTargets[8];
		RenderTarget						DepthTarget;

		class AccelerationStructure*				AS;

	public:
		Shader() = default;
		Shader(const ShaderSpec& spec);

		~Shader();

		void SetComputeRootParameters(ID3D12GraphicsCommandList* cmd);
		void SetGraphicsRootParameters(ID3D12GraphicsCommandList* cmd);

		void SetConstant(const char* parameterName, const void* data);
		void SetTexture(const char* parameterName, Handle<class Texture> texture, uint32 mipLevel = ~0);
		void SetBuffer(const char* parameterName, Handle<class Buffer> buffer);
		void SetSampler(const char* parameterName, Handle<class Sampler> sampler);
		void SetAccelerationStructure(const char* parameterName, class AccelerationStructure* accelerationStructure);

		void ResizeRenderTargets(uint32 width, uint32 height);
		void ReloadShader();

	private:
		void CreateInputLayout(const SC::Kernel& vs, InputLayout& outInputLayout);
		void CreateRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags, SC::Kernel* kernels, uint32 kernelsCount);
		void CreateComputePipeline(ID3D12Device* device, const ShaderSpec& spec);
		void CreateRenderTargets(const ShaderSpec& spec, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
		void CreateGraphicsPipeline(ID3D12Device* device, const ShaderSpec& spec, bool bIsReloading);
		void CreateRayTracingState(ID3D12Device5* device, const ShaderSpec& spec);

		D3D12_RENDER_TARGET_BLEND_DESC GetDefaultBlendDesc();
		D3D12_RENDER_TARGET_BLEND_DESC GetDefaultEnabledBlendDesc();

		std::string_view ParameterTypeToStr(ParameterType type);
	};
}
