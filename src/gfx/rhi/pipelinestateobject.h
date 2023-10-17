#pragma once

#include "staticstates.h"
#include "shader.h"
#include "rootsignature.h"
#include "core/array.h"
#include "core/refcountptr.h"

#include <vector>
#include <string>

namespace limbo::RHI
{
	class PipelineStateSpec
	{
	private:
		friend class PipelineStateObject;

		using TInputLayout = std::vector<D3D12_INPUT_ELEMENT_DESC>;

		RootSignatureHandle							m_RootSignature;
		TInputLayout								m_InputLayout;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE				m_Topology;
		D3D12_RENDER_TARGET_BLEND_DESC				m_BlendDesc;
		D3D12_RASTERIZER_DESC						m_RasterizerDesc;
		D3D12_DEPTH_STENCIL_DESC					m_DepthStencilDesc;

		ShaderHandle								m_VertexShader;
		ShaderHandle								m_PixelShader;
		ShaderHandle								m_ComputeShader;

		TStaticArray<Format, MAX_RENDER_TARGETS>	m_RenderTargetFormats;
		Format										m_DepthTargetFormat;

		std::string									m_Name;

	public:
		PipelineStateSpec& Init();
		PipelineStateSpec& SetRootSignature(RootSignatureHandle rootSignature);
		PipelineStateSpec& SetInputLayout(TInputLayout inputLayout);
		PipelineStateSpec& SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology);
		PipelineStateSpec& SetBlendDesc(const D3D12_RENDER_TARGET_BLEND_DESC& desc);
		PipelineStateSpec& SetRasterizerDesc(const D3D12_RASTERIZER_DESC& desc);
		PipelineStateSpec& SetDepthStencilDesc(const D3D12_DEPTH_STENCIL_DESC& desc);
		PipelineStateSpec& SetRenderTargetFormats(const Span<Format>& rtFormats, Format depthFormat);
		PipelineStateSpec& SetName(const std::string_view& name);
		PipelineStateSpec& SetVertexShader(ShaderHandle vertexShader);
		PipelineStateSpec& SetPixelShader(ShaderHandle pixelShader);
		PipelineStateSpec& SetComputeShader(ShaderHandle computeShader);
	};

	class RTLibSpec
	{
		friend class PipelineStateObject;

		std::vector<D3D12_HIT_GROUP_DESC>	m_HitGroupDescs;
		std::vector<D3D12_EXPORT_DESC>		m_Exports;

	public:
		RTLibSpec& Init();
		RTLibSpec& AddHitGroup(const wchar_t* hitGroupExport,
									   const wchar_t* anyHitShaderImport = nullptr,
									   const wchar_t* closestHitShaderImport = nullptr,
									   const wchar_t* intersectionShaderImport = nullptr,
									   D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES);

		RTLibSpec& AddExport(const wchar_t* name);
	};

	class RTPipelineStateSpec
	{
	private:
		friend class PipelineStateObject;

		std::string							m_Name;
		RootSignatureHandle					m_RootSignature;
		D3D12_RAYTRACING_SHADER_CONFIG		m_ShaderConfig;
		uint32								m_MaxTraceRecursionDepth;

		uint32								m_LibsNum;
		TStaticArray<ShaderHandle, 3>		m_Libs;
		TStaticArray<RTLibSpec, 3>			m_LibsDescs;

	public:
		RTPipelineStateSpec& Init();
		RTPipelineStateSpec& SetGlobalRootSignature(RootSignatureHandle rootSignature);
		RTPipelineStateSpec& AddLib(ShaderHandle lib, const RTLibSpec& libDesc);
		RTPipelineStateSpec& SetShaderConfig(uint32 maxPayloadSizeInBytes, uint32 maxAttributeSizeInBytes);
		RTPipelineStateSpec& SetMaxTraceRecursionDepth(uint32 maxTraceRecursionDepth);
		RTPipelineStateSpec& SetName(const std::string_view& name);
	};

	class PipelineStateObject
	{
	public:
		PipelineStateObject() = default;
		explicit PipelineStateObject(const PipelineStateSpec& spec);
		explicit PipelineStateObject(const RTPipelineStateSpec& spec);

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

		RootSignatureHandle GetRootSignature() const
		{
			return m_RootSignature;
		}

	private:
		void CreateGraphicsPSO(const PipelineStateSpec& spec);
		void CreateComputePSO(const PipelineStateSpec& spec);
		void CreateRTPSO(const RTPipelineStateSpec& spec);

	private:
		RefCountPtr<ID3D12PipelineState>	m_PipelineState;
		RefCountPtr<ID3D12StateObject>		m_StateObject; // for RayTracing
		RootSignatureHandle					m_RootSignature;
		DelegateHandle						m_OnReloadShadersHandle;

		bool m_Compute = false;
	};
	typedef TypedHandle<PipelineStateObject> PSOHandle;
}

