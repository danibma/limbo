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
		m_RootSignatureHandle = rootSignature;
		m_Stream.pRootSignature = RM_GET(m_RootSignatureHandle)->Get();
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetInputLayout(TInputLayout inputLayout)
	{
		m_InputLayout = inputLayout;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology)
	{
		m_Stream.PrimitiveTopologyType = topology;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetBlendDesc(const D3D12_RENDER_TARGET_BLEND_DESC& desc)
	{
		CD3DX12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.RenderTarget[0] = desc;

		m_Stream.BlendState = blendState;
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetRasterizerDesc(const D3D12_RASTERIZER_DESC& desc)
	{
		m_Stream.RasterizerState = CD3DX12_RASTERIZER_DESC(desc);
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetDepthStencilDesc(const D3D12_DEPTH_STENCIL_DESC1& desc)
	{
		m_Stream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(desc);
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetRenderTargetFormats(const Span<Format>& rtFormats, Format depthFormat)
	{
		D3D12_RT_FORMAT_ARRAY rt = {};
		rt.NumRenderTargets = rtFormats.GetSize();
		for (uint32 i = 0; i < rtFormats.GetSize(); ++i)
			rt.RTFormats[i] = D3DFormat(rtFormats[i]);
		m_Stream.RTVFormats = rt;
		m_Stream.DSVFormat = D3DFormat(depthFormat);
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetName(const std::string_view& name)
	{
		m_Name = name;
		return *this;
	}

	void PipelineStateSpec::SetShaderBytecode(ShaderHandle shader, D3D12_SHADER_BYTECODE& bytecode)
	{
		Shader* pShader = RM_GET(shader);

		bytecode = {
			.pShaderBytecode = pShader->Bytecode->GetBufferPointer(),
			.BytecodeLength = pShader->Bytecode->GetBufferSize()
		};
	}

	PipelineStateSpec& PipelineStateSpec::SetVertexShader(ShaderHandle vertexShader)
	{
		SetShaderBytecode(vertexShader, m_Stream.VS);
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetMeshShader(ShaderHandle meshShader)
	{
		SetShaderBytecode(meshShader, m_Stream.MS);
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetAmplificationShader(ShaderHandle ampShader)
	{
		SetShaderBytecode(ampShader, m_Stream.AS);
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetPixelShader(ShaderHandle pixelShader)
	{
		SetShaderBytecode(pixelShader, m_Stream.PS);
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::SetComputeShader(ShaderHandle computeShader)
	{
		m_bIsCompute = true;
		SetShaderBytecode(computeShader, m_Stream.CS);
		return *this;
	}

	PipelineStateSpec& PipelineStateSpec::Init()
	{
		m_Name = "";
		m_Stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		m_Stream.RasterizerState = CD3DX12_RASTERIZER_DESC(TStaticRasterizerState<>::GetRHI());
		m_Stream.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(TStaticDepthStencilState<>::GetRHI());
		m_Stream.DSVFormat = DXGI_FORMAT_UNKNOWN;

		CD3DX12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.RenderTarget[0] = TStaticBlendState<>::GetRHI();
		m_Stream.BlendState = blendState;

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
		: m_Spec(spec)
	{
		CreatePSO();
		m_OnReloadShadersHandle = OnReloadShaders.AddLambda([this]()
		{
			CreatePSO();
		});
	}

	PipelineStateObject::PipelineStateObject(const RTPipelineStateSpec& spec)
		: m_RTSpec(spec)
	{
		check(spec.m_LibsNum > 0);
		check(spec.m_RootSignature.IsValid());

		m_OnReloadShadersHandle = OnReloadShaders.AddLambda([this]()
		{
			CreateRayTracingPSO();
		});

		CreateRayTracingPSO();
	}

	void PipelineStateObject::CreatePSO()
	{
		m_RootSignature = m_Spec.m_RootSignatureHandle;
		m_Compute = m_Spec.m_bIsCompute;

		D3D12_INPUT_LAYOUT_DESC inputLayout = { m_Spec.m_InputLayout.data(), (uint32)m_Spec.m_InputLayout.size() };
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		D3D12_STREAM_OUTPUT_DESC streamOutput = { nullptr };

		m_Spec.m_Stream.InputLayout = inputLayout;
		m_Spec.m_Stream.SampleMask = 0xFFFFFFFF;
		m_Spec.m_Stream.SampleDesc = sampleDesc;
		m_Spec.m_Stream.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
		m_Spec.m_Stream.StreamOutput = streamOutput;
		m_Spec.m_Stream.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
		m_Spec.m_Stream.NodeMask = 0;

		D3D12_PIPELINE_STATE_STREAM_DESC psoStream = {};
		psoStream.SizeInBytes = sizeof(decltype(m_Spec.m_Stream));
		psoStream.pPipelineStateSubobjectStream = &m_Spec.m_Stream;

		DX_CHECK(Device::Ptr->GetDevice()->CreatePipelineState(&psoStream, IID_PPV_ARGS(m_PipelineState.ReleaseAndGetAddressOf())));

		if (!m_Spec.m_Name.empty())
		{
			std::wstring wName;
			Utils::StringConvert(m_Spec.m_Name, wName);
			m_PipelineState->SetName(wName.c_str());
		}
	}

	void PipelineStateObject::CreateRayTracingPSO()
	{
		if (!RHI::GetGPUInfo().bSupportsRaytracing) return;

		m_RootSignature = m_RTSpec.m_RootSignature;

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
		for (uint32 i = 0; i < m_RTSpec.m_LibsNum; ++i)
		{
			ShaderHandle lib = m_RTSpec.m_Libs[i];
			Shader* pLib = RM_GET(lib);

			D3D12_DXIL_LIBRARY_DESC* dxilLib = stream.ContentData.Allocate<D3D12_DXIL_LIBRARY_DESC>();
			dxilLib->DXILLibrary = { pLib->Bytecode->GetBufferPointer(), pLib->Bytecode->GetBufferSize() };
			dxilLib->NumExports = (uint32)m_RTSpec.m_LibsDescs[i].m_Exports.size();
			dxilLib->pExports = const_cast<D3D12_EXPORT_DESC*>(m_RTSpec.m_LibsDescs[i].m_Exports.data());

			AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, dxilLib);

			for (const D3D12_HIT_GROUP_DESC& desc : m_RTSpec.m_LibsDescs[i].m_HitGroupDescs)
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

		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &m_RTSpec.m_ShaderConfig);

		// According to Microsoft: Set max recursion depth as low as needed as drivers may apply optimization strategies for low recursion depths. 
		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {
			.MaxTraceRecursionDepth = m_RTSpec.m_MaxTraceRecursionDepth
		};
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

		D3D12_GLOBAL_ROOT_SIGNATURE globalRS = { .pGlobalRootSignature = RM_GET(m_RTSpec.m_RootSignature)->Get() };
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRS);

		D3D12_STATE_OBJECT_DESC desc = {
			.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
			.NumSubobjects = numObjects,
			.pSubobjects = (D3D12_STATE_SUBOBJECT*)stream.StateObjectData.GetData()
		};
		DX_CHECK(Device::Ptr->GetDevice()->CreateStateObject(&desc, IID_PPV_ARGS(m_StateObject.ReleaseAndGetAddressOf())));

		if (!m_RTSpec.m_Name.empty())
		{
			std::wstring wName;
			Utils::StringConvert(m_RTSpec.m_Name, wName);
			m_StateObject->SetName(wName.c_str());
		}
	}

	PipelineStateObject::~PipelineStateObject()
	{
		OnReloadShaders.Remove(m_OnReloadShadersHandle);
	}
}
