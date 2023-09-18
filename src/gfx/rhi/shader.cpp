#include "stdafx.h"
#include "shader.h"

#include "device.h"
#include "core/window.h"
#include "shadercompiler.h"
#include "resourcemanager.h"
#include "accelerationstructure.h"
#include "rootsignature.h"

#include <dxcapi.h>
#include <format>

#include <array>

#include <d3d12/d3dx12/d3dx12.h>

namespace limbo::RHI
{
	class StateObjectStream
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

	Shader::Shader(const ShaderSpec& spec)
		: m_Name(spec.ProgramName), m_Spec(spec), RTSize(spec.RTSize)
	{
		RootSignature = spec.RootSignature;
		check(RootSignature);

		Device* device = Device::Ptr;
		ID3D12Device5* d3ddevice = device->GetDevice();

		Type = spec.Type;

		m_ReloadShaderDelHandle = device->ReloadShaders.AddRaw(this, &Shader::ReloadShader);

		if (spec.Type == ShaderType::Compute)
			CreateComputePipeline(d3ddevice, spec);
		else if (spec.Type == ShaderType::Graphics)
			CreateGraphicsPipeline(d3ddevice, spec, false);
		else if (spec.Type == ShaderType::RayTracing)
			CreateRayTracingState(d3ddevice, spec);
	}

	Shader::~Shader()
	{
		for (uint8 i = 0; i < RTCount; ++i)
		{
			if (RenderTargets[i].bIsExternal) continue;
			DestroyTexture(RenderTargets[i].Texture);
		}

		if (DepthTarget.Texture.IsValid() && !DepthTarget.bIsExternal)
			DestroyTexture(DepthTarget.Texture);

		Device::Ptr->ReloadShaders.Remove(m_ReloadShaderDelHandle);
	}

	void Shader::Bind(ID3D12GraphicsCommandList* cmd)
	{
		if (Type == ShaderType::Graphics)
			cmd->SetGraphicsRootSignature(RootSignature->Get());
		else // Compute and Ray Tracing
			cmd->SetComputeRootSignature(RootSignature->Get());
	}

	void Shader::CreateComputePipeline(ID3D12Device* device, const ShaderSpec& spec)
	{
		FAILIF(!spec.ProgramName);

		SC::Kernel cs;
		FAILIF(!SC::Compile(cs, spec.ProgramName, spec.CsEntryPoint, SC::KernelType::Compute));

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		D3D12_SHADER_BYTECODE shaderByteCode = {
			.pShaderBytecode = cs.Bytecode->GetBufferPointer(),
			.BytecodeLength = cs.Bytecode->GetBufferSize()
		};

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			.pRootSignature = RootSignature->Get(),
			.CS = shaderByteCode,
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
		DX_CHECK(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PipelineState)));
	}

	void Shader::CreateRayTracingState(ID3D12Device5* device, const ShaderSpec& spec)
	{
		FAILIF(!spec.ProgramName);

		StateObjectStream stream;
		uint32 numObjects = 0;

		auto AddSubobject = [&stream, &numObjects](D3D12_STATE_SUBOBJECT_TYPE type, auto* data)
		{
			D3D12_STATE_SUBOBJECT* subobject = stream.StateObjectData.Allocate<D3D12_STATE_SUBOBJECT>();
			subobject->Type = type;
			subobject->pDesc = data;
			numObjects++;
		};

		// Get all the libs, to later create the global root signature
		uint32 numLibs = (uint32)spec.Libs.size();
		SC::Kernel* libsKernels = stream.ContentData.Allocate<SC::Kernel>(numLibs);
		for (uint32 i = 0; i < numLibs; ++i)
			FAILIF(!SC::Compile(libsKernels[i], spec.Libs[i].LibName, "", SC::KernelType::Lib));

		for (uint32 i = 0; i < numLibs; ++i)
		{
			const ShaderSpec::RaytracingLib& lib = spec.Libs[i];

			D3D12_DXIL_LIBRARY_DESC* dxilLib = stream.ContentData.Allocate<D3D12_DXIL_LIBRARY_DESC>();
			dxilLib->DXILLibrary = { libsKernels[i].Bytecode->GetBufferPointer(), libsKernels[i].Bytecode->GetBufferSize()};
			dxilLib->NumExports  = (uint32)lib.Exports.size();
			dxilLib->pExports	 = const_cast<D3D12_EXPORT_DESC*>(lib.Exports.data());

			AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, dxilLib);

			for (const D3D12_HIT_GROUP_DESC& desc : lib.HitGroupsDescriptions)
			{
				D3D12_HIT_GROUP_DESC* hitGroup	= stream.ContentData.Allocate<D3D12_HIT_GROUP_DESC>();
				hitGroup->HitGroupExport			= desc.HitGroupExport;
				hitGroup->Type						= desc.Type;
				hitGroup->AnyHitShaderImport		= desc.AnyHitShaderImport;
				hitGroup->ClosestHitShaderImport	= desc.ClosestHitShaderImport;
				hitGroup->IntersectionShaderImport	= desc.IntersectionShaderImport;
				AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, hitGroup);
			}
		}

		D3D12_RAYTRACING_SHADER_CONFIG* shaderConfig = stream.ContentData.Allocate<D3D12_RAYTRACING_SHADER_CONFIG>();
		shaderConfig->MaxPayloadSizeInBytes = spec.ShaderConfig.MaxPayloadSizeInBytes;
		shaderConfig->MaxAttributeSizeInBytes = spec.ShaderConfig.MaxAttributeSizeInBytes;
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, shaderConfig);

		// According to Microsoft: Set max recursion depth as low as needed as drivers may apply optimization strategies for low recursion depths. 
		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {
			.MaxTraceRecursionDepth = 1
		};
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

		D3D12_GLOBAL_ROOT_SIGNATURE globalRS = { .pGlobalRootSignature = RootSignature->Get() };
		AddSubobject(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, &globalRS);

		D3D12_STATE_OBJECT_DESC desc = {
			.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,
			.NumSubobjects = numObjects,
			.pSubobjects = (D3D12_STATE_SUBOBJECT*)stream.StateObjectData.GetData()
		};
		DX_CHECK(device->CreateStateObject(&desc, IID_PPV_ARGS(&StateObject)));
	}

	void Shader::CreateRenderTargets(const ShaderSpec& spec, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
	{
		auto createRenderTarget = [this, &spec](int index)
		{
			std::string debugName;
			if (spec.RTFormats[index].DebugName[0] != '\0')
				debugName = std::format("{} - {}", spec.ProgramName, spec.RTFormats[index].DebugName);
			else
				debugName = std::format("{} RT {}", spec.ProgramName, index);

			return CreateTexture({
				.Width  = spec.RTSize.x != 0 ? spec.RTSize.x : GetBackbufferWidth(),
				.Height = spec.RTSize.y != 0 ? spec.RTSize.y : GetBackbufferHeight(),
				.DebugName = debugName.c_str(),
				.Flags = TextureUsage::RenderTarget | TextureUsage::ShaderResource,
				.ClearValue = {
					.Format = D3DFormat(spec.RTFormats[index].RTFormat),
					.Color = { 0.0f, 0.0f, 0.0f, 0.0f }
				},
				.Format = spec.RTFormats[index].RTFormat,
				.Type = TextureType::Texture2D,
			});
		};

		Device::Ptr->OnResizedSwapchain.AddRaw(this, &Shader::ResizeRenderTargets);

		RTCount = 0;
		for (uint8 i = 0; spec.RTFormats[i].IsValid() || spec.RTFormats[i].RTTexture.IsValid(); ++i)
			RTCount++;
		if (RTCount <= 0 && !spec.DepthFormat.IsValid())
		{
			desc.RTVFormats[0]		= D3DFormat(Device::Ptr->GetSwapchainFormat());
			desc.DSVFormat			= D3DFormat(Device::Ptr->GetSwapchainDepthFormat());
			desc.NumRenderTargets	= 1;
			UseSwapchainRT			= true;
		}
		else
		{
			UseSwapchainRT = false;

			for (uint8 i = 0; i < RTCount; ++i)
			{
				desc.RTVFormats[i] = D3DFormat(spec.RTFormats[i].RTFormat);
				if (spec.RTFormats[i].RTTexture.IsValid())
				{
					RenderTargets[i] = {
						.Texture = spec.RTFormats[i].RTTexture,
						.LoadRenderPassOp = spec.RTFormats[i].LoadRenderPassOp,
						.bIsExternal = true
					};
				}
				else
				{
					RenderTargets[i] = {
						.Texture = createRenderTarget(i),
						.LoadRenderPassOp = spec.RTFormats[i].LoadRenderPassOp,
						.bIsExternal = false
					};
					
				}
			}
			desc.NumRenderTargets = RTCount;

			if (!spec.DepthFormat.IsValid() && !spec.DepthFormat.RTTexture.IsValid())
			{
				desc.DepthStencilState.DepthEnable = false;
			}
			else
			{
				desc.DSVFormat = D3DFormat(spec.DepthFormat.RTFormat);

				if (spec.DepthFormat.RTTexture.IsValid())
				{
					DepthTarget = {
						.Texture = spec.DepthFormat.RTTexture,
						.LoadRenderPassOp = spec.DepthFormat.LoadRenderPassOp,
						.bIsExternal = true
					};
				}
				else
				{
					std::string debugName;
					if (spec.DepthFormat.DebugName[0] != '\0')
						debugName = std::format("{} - {}", spec.ProgramName, spec.DepthFormat.DebugName);
					else
						debugName = std::format("{} DSV", spec.ProgramName);

					DepthTarget = {
						.Texture = CreateTexture({
							.Width  = spec.RTSize.x != 0 ? spec.RTSize.x : GetBackbufferWidth(),
							.Height = spec.RTSize.y != 0 ? spec.RTSize.y : GetBackbufferHeight(),
							.DebugName = debugName.c_str(),
							.Flags = TextureUsage::DepthStencil | TextureUsage::ShaderResource,
							.ClearValue = {
								.Format = D3DFormat(spec.DepthFormat.RTFormat),
								.DepthStencil = {
									.Depth = 1.0f,
									.Stencil = 0
								}
							},
							.Format = spec.DepthFormat.RTFormat,
							.Type = TextureType::Texture2D,
						}),
						.LoadRenderPassOp = RenderPassOp::Clear,
						.bIsExternal = false
					};
				}
			}
		}
	}

	void Shader::CreateGraphicsPipeline(ID3D12Device* device, const ShaderSpec& spec, bool bIsReloading)
	{
		FAILIF(!spec.ProgramName);

		SC::Kernel vs;
		FAILIF(!SC::Compile(vs, spec.ProgramName, spec.VSEntryPoint, SC::KernelType::Vertex));
		SC::Kernel ps;
		FAILIF(!SC::Compile(ps, spec.ProgramName, spec.PSEntryPoint, SC::KernelType::Pixel));

		D3D12_SHADER_BYTECODE vsBytecode = {
			.pShaderBytecode = vs.Bytecode->GetBufferPointer(),
			.BytecodeLength = vs.Bytecode->GetBufferSize()
		};

		D3D12_SHADER_BYTECODE psBytecode = {
			.pShaderBytecode = ps.Bytecode->GetBufferPointer(),
			.BytecodeLength = ps.Bytecode->GetBufferSize()
		};

		D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		rasterizerDesc.CullMode = spec.CullMode;
		rasterizerDesc.FrontCounterClockwise = true;
		rasterizerDesc.DepthClipEnable = spec.DepthClip;

		D3D12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.RenderTarget[0] = GetDefaultEnabledBlendDesc();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
			.pRootSignature = RootSignature->Get(),
			.VS = vsBytecode,
			.PS = psBytecode,
			.StreamOutput = nullptr,
			.BlendState = blendState,
			.SampleMask = 0xFFFFFFFF,
			.RasterizerState = rasterizerDesc,
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.InputLayout = {
				spec.InputLayout.data(),
				(uint32)spec.InputLayout.size(),
			},
			.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
			.PrimitiveTopologyType = spec.Topology,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0 
			},
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};

		if (!bIsReloading)
		{
			CreateRenderTargets(spec, desc);
		}
		else
		{
			if (RTCount <= 0 && !spec.DepthFormat.IsValid())
			{
				desc.RTVFormats[0] = D3DFormat(Device::Ptr->GetSwapchainFormat());
				desc.DSVFormat = D3DFormat(Device::Ptr->GetSwapchainDepthFormat());
				desc.NumRenderTargets = 1;
				UseSwapchainRT = true;
			}
			else
			{
				UseSwapchainRT = false;

				for (uint8 i = 0; i < RTCount; ++i)
					desc.RTVFormats[i] = D3DFormat(spec.RTFormats[i].RTFormat);
				desc.NumRenderTargets = RTCount;

				if (!spec.DepthFormat.IsValid() && !spec.DepthFormat.RTTexture.IsValid())
					desc.DepthStencilState.DepthEnable = false;
				else
					desc.DSVFormat = D3DFormat(spec.DepthFormat.RTFormat);
			}
		}

		DX_CHECK(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&PipelineState)));
	}

	D3D12_RENDER_TARGET_BLEND_DESC Shader::GetDefaultBlendDesc()
	{
		return {
			.BlendEnable = false,
			.LogicOpEnable = false,
			.SrcBlend = D3D12_BLEND_ONE,
			.DestBlend = D3D12_BLEND_ZERO,
			.BlendOp = D3D12_BLEND_OP_ADD,
			.SrcBlendAlpha = D3D12_BLEND_ONE,
			.DestBlendAlpha = D3D12_BLEND_ZERO,
			.BlendOpAlpha = D3D12_BLEND_OP_ADD,
			.LogicOp = D3D12_LOGIC_OP_NOOP,
			.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
		};
	}

	D3D12_RENDER_TARGET_BLEND_DESC Shader::GetDefaultEnabledBlendDesc()
	{
		return {
			.BlendEnable = true,
			.LogicOpEnable = false,
			.SrcBlend = D3D12_BLEND_SRC_ALPHA,
			.DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
			.BlendOp = D3D12_BLEND_OP_ADD,
			.SrcBlendAlpha = D3D12_BLEND_ONE,
			.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA,
			.BlendOpAlpha = D3D12_BLEND_OP_ADD,
			.LogicOp = D3D12_LOGIC_OP_NOOP,
			.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
		};
	}

	void Shader::ResizeRenderTargets(uint32 width, uint32 height)
	{
		if (UseSwapchainRT) return;

		for (uint8 i = 0; i < RTCount; ++i)
		{
			if (!RenderTargets[i].Texture.IsValid()) continue;
			if (RenderTargets[i].bIsExternal) continue;

			Texture* texture = ResourceManager::Ptr->GetTexture(RenderTargets[i].Texture);
			texture->ReloadSize(width, height);
		}

		if (DepthTarget.Texture.IsValid() && !DepthTarget.bIsExternal)
		{
			Texture* texture = ResourceManager::Ptr->GetTexture(DepthTarget.Texture);
			texture->ReloadSize(width, height);
		}
	}

	void Shader::ReloadShader()
	{
		PipelineState.Reset();
		RootSignature->Reset();

		if (Type == ShaderType::Compute)
			CreateComputePipeline(Device::Ptr->GetDevice(), m_Spec);
		else if (Type == ShaderType::Graphics)
			CreateGraphicsPipeline(Device::Ptr->GetDevice(), m_Spec, true);
		else if (Type == ShaderType::RayTracing)
			CreateRayTracingState(Device::Ptr->GetDevice(), m_Spec);
	}
}
