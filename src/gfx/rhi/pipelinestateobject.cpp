#include "stdafx.h"
#include "pipelinestateobject.h"

#include "device.h"
#include "rootsignature.h"

#include <span>

namespace limbo::RHI
{
	constexpr D3D12_ROOT_SIGNATURE_FLAGS ComputeRSFlags = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
														  D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
														  D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
														  D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
														  D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
														  D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
														  D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
	
	constexpr D3D12_ROOT_SIGNATURE_FLAGS GraphicsRSFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
														   D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
														   D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
														   D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
														   D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
														   D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

	class RayTracingStateObjectStream
	{
		template<size_t SIZE>
		struct DataAllocator
		{
			template<typename T>
			T* Allocate(uint32 count = 1)
			{
				check(m_Offset + count * sizeof(T) <= SIZE);
				T* pData = reinterpret_cast<T*>(&m_Data[m_Offset]);
				m_Offset += count * sizeof(T);
				return pData;
			}
			void Reset() { m_Offset = 0; }
			const void* GetData() const { return m_Data.data(); }
			size_t Size() const { return m_Offset; }
		private:
			size_t m_Offset = 0;
			std::array<uint8, SIZE> m_Data = {};
		};

	public:
		DataAllocator<1 << 8>  StateObjectData = {};
		DataAllocator<1 << 10> ContentData = {};
	};

	void PipelineStateInitializer::SetRootSignature(RootSignature* rootSignature)
	{
		m_RootSignature = rootSignature;
	}

	void PipelineStateInitializer::SetInputLayout(TInputLayout inputLayout)
	{
		m_InputLayout = inputLayout;
	}

	void PipelineStateInitializer::SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology)
	{
		m_Topology = topology;
	}

	void PipelineStateInitializer::SetBlendDesc(const D3D12_RENDER_TARGET_BLEND_DESC& desc)
	{
		m_BlendDesc = desc;
	}

	void PipelineStateInitializer::SetRasterizerDesc(const D3D12_RASTERIZER_DESC& desc)
	{
		m_RasterizerDesc = desc;
	}

	void PipelineStateInitializer::SetDepthStencilDesc(const D3D12_DEPTH_STENCIL_DESC& desc)
	{
		m_DepthStencilDesc = desc;
	}

	void PipelineStateInitializer::SetRenderTargetFormats(const Span<Format>& rtFormats, Format depthFormat)
	{
		m_RenderTargetFormats = rtFormats;
		m_DepthTargetFormat = depthFormat;
	}

	void PipelineStateInitializer::SetName(const std::string_view& name)
	{
		m_Name = name;
	}

	void RaytracingLibDesc::AddHitGroup(const wchar_t* hitGroupExport, const wchar_t* anyHitShaderImport /*= L""*/, const wchar_t* closestHitShaderImport /*= L""*/, const wchar_t* intersectionShaderImport /*= L""*/, D3D12_HIT_GROUP_TYPE type /*= D3D12_HIT_GROUP_TYPE_TRIANGLES*/)
	{
		m_HitGroupDescs.emplace_back(hitGroupExport, type, anyHitShaderImport, closestHitShaderImport, intersectionShaderImport);
	}

	void RaytracingLibDesc::AddExport(const wchar_t* name)
	{
		m_Exports.emplace_back(name, name, D3D12_EXPORT_FLAG_NONE);
	}

	void RaytracingPipelineStateInitializer::SetGlobalRootSignature(RootSignature* rootSignature)
	{
		m_RootSignature = rootSignature;
	}

	void RaytracingPipelineStateInitializer::AddLib(ShaderHandle lib, const RaytracingLibDesc& libDesc)
	{
		m_Libs[m_LibsNum] = lib;
		m_LibsDescs[m_LibsNum] = libDesc;
		m_LibsNum++;
	}

	void RaytracingPipelineStateInitializer::SetShaderConfig(uint32 maxPayloadSizeInBytes, uint32 maxAttributeSizeInBytes)
	{
		m_ShaderConfig = { maxPayloadSizeInBytes, maxAttributeSizeInBytes };
	}

	void RaytracingPipelineStateInitializer::SetMaxTraceRecursionDepth(uint32 maxTraceRecursionDepth)
	{
		m_MaxTraceRecursionDepth = maxTraceRecursionDepth;
	}

	void RaytracingPipelineStateInitializer::SetName(const std::string_view& name)
	{
		m_Name = name;
	}

	PipelineStateObject::PipelineStateObject(const PipelineStateInitializer& initializer)
	{
		check((initializer.m_ComputeShader.IsValid() && !initializer.m_VertexShader.IsValid() && !initializer.m_PixelShader.IsValid()) ||
			  (!initializer.m_ComputeShader.IsValid() && (initializer.m_VertexShader.IsValid() || initializer.m_PixelShader.IsValid())));

		if (initializer.m_ComputeShader.IsValid())
			CreateComputePSO(initializer);
		else
			CreateGraphicsPSO(initializer);
	}

	PipelineStateObject::PipelineStateObject(const RaytracingPipelineStateInitializer& initializer)
	{
		check(initializer.m_LibsNum > 0);
		check(initializer.m_RootSignature);

		m_RootSignature = initializer.m_RootSignature;

		RayTracingStateObjectStream stream;
		uint32 numObjects = 0;

		auto AddSubobject = [&stream, &numObjects](D3D12_STATE_SUBOBJECT_TYPE type, auto* data)
		{
			D3D12_STATE_SUBOBJECT* subobject = stream.StateObjectData.Allocate<D3D12_STATE_SUBOBJECT>();
			subobject->Type = type;
			subobject->pDesc = data;
			numObjects++;
		};

		// Get all the libs, to later create the global root signature
		for (uint32 i = 0; i < initializer.m_LibsNum; ++i)
		{
			ShaderHandle lib = initializer.m_Libs[i];
			Shader* pLib = RM_GET(lib);

			D3D12_DXIL_LIBRARY_DESC* dxilLib = stream.ContentData.Allocate<D3D12_DXIL_LIBRARY_DESC>();
			dxilLib->DXILLibrary = { pLib->Bytecode->GetBufferPointer(), pLib->Bytecode->GetBufferSize() };
			dxilLib->NumExports = (uint32)initializer.m_LibsDescs[i].m_Exports.size();
			dxilLib->pExports = const_cast<D3D12_EXPORT_DESC*>(initializer.m_LibsDescs[i].m_Exports.data());

			AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, dxilLib);

			for (const D3D12_HIT_GROUP_DESC& desc : initializer.m_LibsDescs[i].m_HitGroupDescs)
			{
				D3D12_HIT_GROUP_DESC* hitGroup = stream.ContentData.Allocate<D3D12_HIT_GROUP_DESC>();
				hitGroup->HitGroupExport = desc.HitGroupExport;
				hitGroup->Type = desc.Type;
				hitGroup->AnyHitShaderImport = desc.AnyHitShaderImport;
				hitGroup->ClosestHitShaderImport = desc.ClosestHitShaderImport;
				hitGroup->IntersectionShaderImport = desc.IntersectionShaderImport;
				AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, hitGroup);
			}
		}

		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &initializer.m_ShaderConfig);

		// According to Microsoft: Set max recursion depth as low as needed as drivers may apply optimization strategies for low recursion depths. 
		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {
			.MaxTraceRecursionDepth = initializer.m_MaxTraceRecursionDepth
		};
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

		D3D12_GLOBAL_ROOT_SIGNATURE globalRS = { .pGlobalRootSignature = initializer.m_RootSignature->Get() };
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRS);

		D3D12_STATE_OBJECT_DESC desc = {
			.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
			.NumSubobjects = numObjects,
			.pSubobjects = (D3D12_STATE_SUBOBJECT*)stream.StateObjectData.GetData()
		};
		DX_CHECK(Device::Ptr->GetDevice()->CreateStateObject(&desc, IID_PPV_ARGS(m_StateObject.ReleaseAndGetAddressOf())));

		if (!initializer.m_Name.empty())
		{
			std::wstring wName;
			Utils::StringConvert(initializer.m_Name, wName);
			m_StateObject->SetName(wName.c_str());
		}
	}

	void PipelineStateObject::CreateGraphicsPSO(const PipelineStateInitializer& initializer)
	{
		check(initializer.m_RootSignature);

		D3D12_SHADER_BYTECODE vsBytecode = {};
		D3D12_SHADER_BYTECODE psBytecode = {};

		m_RootSignature = initializer.m_RootSignature;

		if (initializer.m_VertexShader.IsValid())
		{
			Shader* pShader = RM_GET(initializer.m_VertexShader);

			vsBytecode = {
				.pShaderBytecode = pShader->Bytecode->GetBufferPointer(),
				.BytecodeLength = pShader->Bytecode->GetBufferSize()
			};
		}

		if (initializer.m_PixelShader.IsValid())
		{
			Shader* pShader = RM_GET(initializer.m_PixelShader);

			psBytecode = {
				.pShaderBytecode = pShader->Bytecode->GetBufferPointer(),
				.BytecodeLength = pShader->Bytecode->GetBufferSize()
			};
		}

		D3D12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.RenderTarget[0] = initializer.m_BlendDesc;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
			.pRootSignature = m_RootSignature->Get(),
			.VS = vsBytecode,
			.PS = psBytecode,
			.StreamOutput = nullptr,
			.BlendState = blendState,
			.SampleMask = 0xFFFFFFFF,
			.RasterizerState = initializer.m_RasterizerDesc,
			.DepthStencilState = initializer.m_DepthStencilDesc,
			.InputLayout = {
				initializer.m_InputLayout.data(),
				(uint32)initializer.m_InputLayout.size(),
			},
			.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
			.PrimitiveTopologyType = initializer.m_Topology,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};

		desc.NumRenderTargets = initializer.m_RenderTargetFormats.GetSize();

		for (uint32 i = 0; i < desc.NumRenderTargets; i++)
			desc.RTVFormats[i] = D3DFormat(initializer.m_RenderTargetFormats[i]);

		check(initializer.m_DepthStencilDesc.DepthEnable == (initializer.m_DepthTargetFormat != Format::UNKNOWN));
		desc.DSVFormat = D3DFormat(Device::Ptr->GetSwapchainDepthFormat());

		DX_CHECK(Device::Ptr->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));

		if (!initializer.m_Name.empty())
		{
			std::wstring wName;
			Utils::StringConvert(initializer.m_Name, wName);
			m_PipelineState->SetName(wName.c_str());
		}
	}

	void PipelineStateObject::CreateComputePSO(const PipelineStateInitializer& initializer)
	{
		check(initializer.m_ComputeShader.IsValid());
		check(initializer.m_RootSignature);

		m_RootSignature = initializer.m_RootSignature;

		Shader* pShader = RM_GET(initializer.m_ComputeShader);

		D3D12_SHADER_BYTECODE shaderByteCode = {
			.pShaderBytecode = pShader->Bytecode->GetBufferPointer(),
			.BytecodeLength = pShader->Bytecode->GetBufferSize()
		};

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			.pRootSignature = m_RootSignature->Get(),
			.CS = shaderByteCode,
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
		DX_CHECK(Device::Ptr->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));

		if (!initializer.m_Name.empty())
		{
			std::wstring wName;
			Utils::StringConvert(initializer.m_Name, wName);
			m_PipelineState->SetName(wName.c_str());
		}

		m_Compute = true;
	}

	PipelineStateObject::~PipelineStateObject()
	{
	}
}
