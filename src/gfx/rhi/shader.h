#pragma once

#include "definitions.h"
#include "resourcepool.h"

namespace limbo::RHI
{
	class RootSignature;
	namespace SC { struct Kernel; }

	struct ShaderSpec
	{
	private:
		using HitGroupDescList = std::vector<D3D12_HIT_GROUP_DESC>;
		using ExportsList = std::vector<D3D12_EXPORT_DESC>;
		using TInputLayout = std::vector<D3D12_INPUT_ELEMENT_DESC>;

	public:
		struct RenderTargetDesc
		{
			Format					RTFormat;
			const char*				DebugName = ""; // If the RTTexture is set, this does nothing

			Handle<class Texture>	RTTexture; // Use an already created texture, eg. from another shader

			RenderPassOp			LoadRenderPassOp = RenderPassOp::Clear;

			bool IsValid() const { return RTFormat != Format::UNKNOWN; }
		};

		struct RaytracingLib
		{
			const char*						LibName;
			HitGroupDescList				HitGroupsDescriptions;
			ExportsList						Exports;
		};

	public:
		const char*						ProgramName;
		const char*						VSEntryPoint = "VSMain";
		const char*						PSEntryPoint = "PSMain";
		const char*						CsEntryPoint = "CSMain";

		TInputLayout					InputLayout;
		RootSignature*					RootSignature = nullptr;

		uint2							RTSize = { 0, 0 }; // If the RTFormats have RTTexture set, this does nothing
		RenderTargetDesc				RTFormats[8];
		RenderTargetDesc				DepthFormat = { Format::UNKNOWN };

		D3D12_PRIMITIVE_TOPOLOGY_TYPE	Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		D3D12_CULL_MODE					CullMode = D3D12_CULL_MODE_BACK;

		bool							DepthClip = true;

		/**
		 * Fill this for raytracing and set the type to ShaderType::RayTracing.
		 * You can set the program name in order to have a debug name for the shader.
		 * The other fields are useless for Ray Tracing shaders
		 */
		std::vector<RaytracingLib>		Libs;
		D3D12_RAYTRACING_SHADER_CONFIG	ShaderConfig;

		ShaderType						Type = ShaderType::Graphics;
	};

	class Shader
	{
		struct RenderTarget
		{
			Handle<class Texture>	Texture;
			RenderPassOp			LoadRenderPassOp;
			bool					bIsExternal = false; // if the render target comes from another shader, this is true
		};

		std::string							m_Name;
		ShaderSpec							m_Spec;
		DelegateHandle						m_ReloadShaderDelHandle;

	public:
		RootSignature*						RootSignature;
		ComPtr<ID3D12PipelineState>			PipelineState;
		ComPtr<ID3D12StateObject>			StateObject; // for RayTracing
		ShaderType							Type;
		bool								UseSwapchainRT;

		uint2								RTSize;
		uint8								RTCount;
		RenderTarget						RenderTargets[8];
		RenderTarget						DepthTarget;

	public:
		Shader() = default;
		Shader(const ShaderSpec& spec);

		~Shader();

		void Bind(ID3D12GraphicsCommandList* cmd);

		void ResizeRenderTargets(uint32 width, uint32 height);
		void ReloadShader();

	private:
		void CreateComputePipeline(ID3D12Device* device, const ShaderSpec& spec);
		void CreateRenderTargets(const ShaderSpec& spec, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc);
		void CreateGraphicsPipeline(ID3D12Device* device, const ShaderSpec& spec, bool bIsReloading);
		void CreateRayTracingState(ID3D12Device5* device, const ShaderSpec& spec);

		D3D12_RENDER_TARGET_BLEND_DESC GetDefaultBlendDesc();
		D3D12_RENDER_TARGET_BLEND_DESC GetDefaultEnabledBlendDesc();
	};
}
