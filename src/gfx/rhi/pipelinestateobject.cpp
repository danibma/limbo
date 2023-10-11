#include "stdafx.h"
#include "pipelinestateobject.h"
#include "device.h"
#include "rootsignature.h"
#include "core/utils.h"
#include "resourcemanager.h"

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
		template<uint32 SIZE>
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
			const void* GetData() const { return m_Data.GetData(); }
			uint32 Size() const { return m_Offset; }
		private:
			uint32 m_Offset = 0;
			TStaticArray<uint8, SIZE> m_Data = {};
		};

	public:
		DataAllocator<1 << 8>  StateObjectData = {};
		DataAllocator<1 << 10> ContentData = {};
	};

	PipelineStateSpec& PipelineStateSpec::SetRootSignature(RootSignatureHandle rootSignature)
	{
		m_RootSignature = rootSignature;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetInputLayout(TInputLayout inputLayout)
	{
		m_InputLayout = inputLayout;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology)
	{
		m_Topology = topology;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetBlendDesc(const D3D12_RENDER_TARGET_BLEND_DESC& desc)
	{
		m_BlendDesc = desc;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetRasterizerDesc(const D3D12_RASTERIZER_DESC& desc)
	{
		m_RasterizerDesc = desc;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetDepthStencilDesc(const D3D12_DEPTH_STENCIL_DESC& desc)
	{
		m_DepthStencilDesc = desc;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetRenderTargetFormats(const Span<Format>& rtFormats, Format depthFormat)
	{
		m_RenderTargetFormats = rtFormats;
		m_DepthTargetFormat = depthFormat;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetName(const std::string_view& name)
	{
		m_Name = name;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetVertexShader(ShaderHandle vertexShader)
	{
		m_VertexShader = vertexShader;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetPixelShader(ShaderHandle pixelShader)
	{
		m_PixelShader = pixelShader;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetComputeShader(ShaderHandle computeShader)
	{
		m_ComputeShader = computeShader;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::Init()
	{
		m_Name = "";
		m_Topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_BlendDesc = TStaticBlendState<>::GetRHI();
		m_RasterizerDesc = TStaticRasterizerState<>::GetRHI();
		m_DepthStencilDesc = TStaticDepthStencilState<>::GetRHI();
		m_DepthTargetFormat = Format::UNKNOWN;

		return *this;
	}

	RTLibSpec& RTLibSpec::AddHitGroup(const wchar_t* hitGroupExport, const wchar_t* anyHitShaderImport /*= L""*/, const wchar_t* closestHitShaderImport /*= L""*/, const wchar_t* intersectionShaderImport /*= L""*/, D3D12_HIT_GROUP_TYPE type /*= D3D12_HIT_GROUP_TYPE_TRIANGLES*/)
	{
		m_HitGroupDescs.emplace_back(hitGroupExport, type, anyHitShaderImport, closestHitShaderImport, intersectionShaderImport);
		return *this;
	}

	RTLibSpec& RTLibSpec::AddExport(const wchar_t* name)
	{
		m_Exports.emplace_back(name, name, D3D12_EXPORT_FLAG_NONE);
		return *this;
	}

	RTLibSpec& RTLibSpec::Init()
	{
		return *this;
	}

	RTPipelineStateSpec& RTPipelineStateSpec::SetGlobalRootSignature(RootSignatureHandle rootSignature)
	{
		m_RootSignature = rootSignature;
		return *this;
	}

	RTPipelineStateSpec& RTPipelineStateSpec::AddLib(ShaderHandle lib, const RTLibSpec& libDesc)
	{
		m_Libs[m_LibsNum] = lib;
		m_LibsDescs[m_LibsNum] = libDesc;
		m_LibsNum++;
		return *this;
	}

	RTPipelineStateSpec& RTPipelineStateSpec::SetShaderConfig(uint32 maxPayloadSizeInBytes, uint32 maxAttributeSizeInBytes)
	{
		m_ShaderConfig = { maxPayloadSizeInBytes, maxAttributeSizeInBytes };
		return *this;
	}

	RTPipelineStateSpec& RTPipelineStateSpec::SetMaxTraceRecursionDepth(uint32 maxTraceRecursionDepth)
	{
		m_MaxTraceRecursionDepth = maxTraceRecursionDepth;
		return *this;
	}

	RTPipelineStateSpec& RTPipelineStateSpec::SetName(const std::string_view& name)
	{
		m_Name = name;
		return *this;
	}

	RTPipelineStateSpec& RTPipelineStateSpec::Init()
	{
		m_Name = "";
		m_MaxTraceRecursionDepth = 1;
		return *this;
	}

	PipelineStateObject::PipelineStateObject(const PipelineStateSpec& spec)
	{
		check((spec.m_ComputeShader.IsValid() && !spec.m_VertexShader.IsValid() && !spec.m_PixelShader.IsValid()) ||
			  (!spec.m_ComputeShader.IsValid() && (spec.m_VertexShader.IsValid() || spec.m_PixelShader.IsValid())));

		if (spec.m_ComputeShader.IsValid())
			CreateComputePSO(spec);
		else
			CreateGraphicsPSO(spec);

		m_OnReloadShadersHandle = OnReloadShaders.AddLambda([spec, this]()
		{
			if (spec.m_ComputeShader.IsValid())
				CreateComputePSO(spec);
			else
				CreateGraphicsPSO(spec);
		});
	}

	PipelineStateObject::PipelineStateObject(const RTPipelineStateSpec& spec)
	{
		check(spec.m_LibsNum > 0);
		check(spec.m_RootSignature.IsValid());

		m_OnReloadShadersHandle = OnReloadShaders.AddLambda([spec, this]()
		{
			CreateRTPSO(spec);
		});

		CreateRTPSO(spec);
	}

	void PipelineStateObject::CreateGraphicsPSO(const PipelineStateSpec& spec)
	{
		check(spec.m_RootSignature.IsValid());

		D3D12_SHADER_BYTECODE vsBytecode = {};
		D3D12_SHADER_BYTECODE psBytecode = {};

		m_RootSignature = spec.m_RootSignature;

		if (spec.m_VertexShader.IsValid())
		{
			Shader* pShader = RM_GET(spec.m_VertexShader);

			vsBytecode = {
				.pShaderBytecode = pShader->Bytecode->GetBufferPointer(),
				.BytecodeLength = pShader->Bytecode->GetBufferSize()
			};
		}

		if (spec.m_PixelShader.IsValid())
		{
			Shader* pShader = RM_GET(spec.m_PixelShader);

			psBytecode = {
				.pShaderBytecode = pShader->Bytecode->GetBufferPointer(),
				.BytecodeLength = pShader->Bytecode->GetBufferSize()
			};
		}

		D3D12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.RenderTarget[0] = spec.m_BlendDesc;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
			.pRootSignature = RM_GET(m_RootSignature)->Get(),
			.VS = vsBytecode,
			.PS = psBytecode,
			.StreamOutput = nullptr,
			.BlendState = blendState,
			.SampleMask = 0xFFFFFFFF,
			.RasterizerState = spec.m_RasterizerDesc,
			.DepthStencilState = spec.m_DepthStencilDesc,
			.InputLayout = {
				spec.m_InputLayout.data(),
				(uint32)spec.m_InputLayout.size(),
			},
			.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
			.PrimitiveTopologyType = spec.m_Topology,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};

		desc.NumRenderTargets = spec.m_RenderTargetFormats.GetSize();

		for (uint32 i = 0; i < desc.NumRenderTargets; i++)
			desc.RTVFormats[i] = D3DFormat(spec.m_RenderTargetFormats[i]);

		check(spec.m_DepthStencilDesc.DepthEnable == (spec.m_DepthTargetFormat != Format::UNKNOWN));
		desc.DSVFormat = D3DFormat(Device::Ptr->GetSwapchainDepthFormat());

		DX_CHECK(Device::Ptr->GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));

		if (!spec.m_Name.empty())
		{
			std::wstring wName;
			Utils::StringConvert(spec.m_Name, wName);
			m_PipelineState->SetName(wName.c_str());
		}
	}

	void PipelineStateObject::CreateComputePSO(const PipelineStateSpec& spec)
	{
		check(spec.m_ComputeShader.IsValid());
		check(spec.m_RootSignature.IsValid());

		m_RootSignature = spec.m_RootSignature;

		Shader* pShader = RM_GET(spec.m_ComputeShader);

		D3D12_SHADER_BYTECODE shaderByteCode = {
			.pShaderBytecode = pShader->Bytecode->GetBufferPointer(),
			.BytecodeLength = pShader->Bytecode->GetBufferSize()
		};

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			.pRootSignature = RM_GET(m_RootSignature)->Get(),
			.CS = shaderByteCode,
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
		DX_CHECK(Device::Ptr->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));

		if (!spec.m_Name.empty())
		{
			std::wstring wName;
			Utils::StringConvert(spec.m_Name, wName);
			m_PipelineState->SetName(wName.c_str());
		}

		m_Compute = true;
	}

	void PipelineStateObject::CreateRTPSO(const RTPipelineStateSpec& spec)
	{
		m_RootSignature = spec.m_RootSignature;

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
		for (uint32 i = 0; i < spec.m_LibsNum; ++i)
		{
			ShaderHandle lib = spec.m_Libs[i];
			Shader* pLib = RM_GET(lib);

			D3D12_DXIL_LIBRARY_DESC* dxilLib = stream.ContentData.Allocate<D3D12_DXIL_LIBRARY_DESC>();
			dxilLib->DXILLibrary = { pLib->Bytecode->GetBufferPointer(), pLib->Bytecode->GetBufferSize() };
			dxilLib->NumExports = (uint32)spec.m_LibsDescs[i].m_Exports.size();
			dxilLib->pExports = const_cast<D3D12_EXPORT_DESC*>(spec.m_LibsDescs[i].m_Exports.data());

			AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, dxilLib);

			for (const D3D12_HIT_GROUP_DESC& desc : spec.m_LibsDescs[i].m_HitGroupDescs)
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

		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &spec.m_ShaderConfig);

		// According to Microsoft: Set max recursion depth as low as needed as drivers may apply optimization strategies for low recursion depths. 
		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {
			.MaxTraceRecursionDepth = spec.m_MaxTraceRecursionDepth
		};
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

		D3D12_GLOBAL_ROOT_SIGNATURE globalRS = { .pGlobalRootSignature = RM_GET(spec.m_RootSignature)->Get() };
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRS);

		D3D12_STATE_OBJECT_DESC desc = {
			.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
			.NumSubobjects = numObjects,
			.pSubobjects = (D3D12_STATE_SUBOBJECT*)stream.StateObjectData.GetData()
		};
		DX_CHECK(Device::Ptr->GetDevice()->CreateStateObject(&desc, IID_PPV_ARGS(m_StateObject.ReleaseAndGetAddressOf())));

		if (!spec.m_Name.empty())
		{
			std::wstring wName;
			Utils::StringConvert(spec.m_Name, wName);
			m_StateObject->SetName(wName.c_str());
		}
	}

	PipelineStateObject::~PipelineStateObject()
	{
		OnReloadShaders.Remove(m_OnReloadShadersHandle);
	}
}
