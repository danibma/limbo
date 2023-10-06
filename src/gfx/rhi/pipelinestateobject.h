#pragma once

#include "shader.h"
#include "resourcepool.h"
#include "staticstates.h"
#include "core/array.h"
#include "core/refcountptr.h"

#include <vector>
#include <string>

namespace limbo::RHI
{
	class RootSignature;

	class PipelineStateInitializer
	{
	private:
		friend class PipelineStateObject;

		using TInputLayout = std::vector<D3D12_INPUT_ELEMENT_DESC>;

		RootSignature*								m_RootSignature;
		TInputLayout								m_InputLayout;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE				m_Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		D3D12_RENDER_TARGET_BLEND_DESC				m_BlendDesc = TStaticBlendState<>::GetRHI();
		D3D12_RASTERIZER_DESC						m_RasterizerDesc = TStaticRasterizerState<>::GetRHI();
		D3D12_DEPTH_STENCIL_DESC					m_DepthStencilDesc = TStaticDepthStencilState<>::GetRHI();

		Handle<Shader>								m_VertexShader;
		Handle<Shader>								m_PixelShader;
		Handle<Shader>								m_ComputeShader;

		TStaticArray<Format, MAX_RENDER_TARGETS>	m_RenderTargetFormats = {};
		Format										m_DepthTargetFormat = Format::UNKNOWN;

		std::string									m_Name = "";

	public:
		void SetRootSignature(RootSignature* rootSignature);
		void SetInputLayout(TInputLayout inputLayout);
		void SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology);
		void SetBlendDesc(const D3D12_RENDER_TARGET_BLEND_DESC& desc);
		void SetRasterizerDesc(const D3D12_RASTERIZER_DESC& desc);
		void SetDepthStencilDesc(const D3D12_DEPTH_STENCIL_DESC& desc);
		void SetRenderTargetFormats(const Span<Format>& rtFormats, Format depthFormat);
		void SetName(const std::string_view& name);

		void SetVertexShader(Handle<Shader> vertexShader)
		{
			m_VertexShader = vertexShader;
		}

		void SetPixelShader(Handle<Shader> pixelShader)
		{
			m_PixelShader = pixelShader;
		}

		void SetComputeShader(Handle<Shader> computeShader)
		{
			m_ComputeShader = computeShader;
		}
	};

	class RaytracingLibDesc
	{
		friend class PipelineStateObject;

		std::vector<D3D12_HIT_GROUP_DESC>	m_HitGroupDescs;
		std::vector<D3D12_EXPORT_DESC>		m_Exports;

	public:
		void AddHitGroup(const wchar_t* hitGroupExport, 
						 const wchar_t* anyHitShaderImport = nullptr,
						 const wchar_t* closestHitShaderImport = nullptr,
						 const wchar_t* intersectionShaderImport = nullptr,
						 D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES);

		void AddExport(const wchar_t* name);
	};

	class RaytracingPipelineStateInitializer
	{
	private:
		friend class PipelineStateObject;

		std::string							m_Name = "";
		RootSignature*						m_RootSignature;
		D3D12_RAYTRACING_SHADER_CONFIG		m_ShaderConfig;
		uint32								m_MaxTraceRecursionDepth = 1;

		uint32								m_LibsNum;
		TStaticArray<Handle<Shader>, 3>		m_Libs;
		TStaticArray<RaytracingLibDesc, 3>	m_LibsDescs;
	public:

		void SetGlobalRootSignature(RootSignature* rootSignature);
		void AddLib(Handle<Shader> lib, const RaytracingLibDesc& libDesc);
		void SetShaderConfig(uint32 maxPayloadSizeInBytes, uint32 maxAttributeSizeInBytes);
		void SetMaxTraceRecursionDepth(uint32 maxTraceRecursionDepth);
		void SetName(const std::string_view& name);
	};

	class PipelineStateObject
	{
	public:
		explicit PipelineStateObject(const PipelineStateInitializer& initializer);
		explicit PipelineStateObject(const RaytracingPipelineStateInitializer& initializer);

		~PipelineStateObject();

		bool IsRaytracing()
		{
			return m_StateObject.IsValid();
		}

		bool IsCompute()
		{
			return m_Compute || IsRaytracing();
		}

		ID3D12PipelineState* GetPipelineState() const
		{
			return m_PipelineState.Get();
		}

		ID3D12StateObject* GetStateObject() const
		{
			return m_StateObject.Get();
		}

		RootSignature* GetRootSignature() const 
		{
			return m_RootSignature;
		}

	private:
		void CreateGraphicsPSO(const PipelineStateInitializer& initializer);
		void CreateComputePSO(const PipelineStateInitializer& initializer);

	private:
		RefCountPtr<ID3D12PipelineState>	m_PipelineState;
		RefCountPtr<ID3D12StateObject>		m_StateObject; // for RayTracing
		RootSignature*						m_RootSignature;

		bool m_Compute = false;
	};
}

