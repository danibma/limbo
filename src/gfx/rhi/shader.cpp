#include "stdafx.h"
#include "shader.h"

#include "device.h"
#include "core/algo.h"
#include "core/window.h"
#include "shadercompiler.h"
#include "resourcemanager.h"

#include <dxcapi.h>
#include <format>

#include <d3d12/d3dx12/d3dx12.h>



namespace limbo::Gfx
{
	Shader::Shader(const ShaderSpec& spec)
		: m_Name(spec.ProgramName), m_Spec(spec)
	{
		Device* device = Device::Ptr;
		ID3D12Device* d3ddevice = device->GetDevice();

		Type = spec.Type;

		m_ReloadShaderDelHandle = device->ReloadShaders.AddRaw(this, &Shader::ReloadShader);

		if (spec.Type == ShaderType::Compute)
			CreateComputePipeline(d3ddevice, spec);
		else if (spec.Type == ShaderType::Graphics)
			CreateGraphicsPipeline(d3ddevice, spec, false);
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

	void Shader::SetComputeRootParameters(ID3D12GraphicsCommandList* cmd)
	{
		for (const auto& [name, parameter] : ParameterMap)
		{
			if (parameter.Type == ParameterType::Constants)
			{
				FAILIF(!parameter.Data);
				cmd->SetComputeRoot32BitConstants(parameter.RPIndex, parameter.NumValues, parameter.Data, parameter.Offset);
			}
			else
			{
				FAILIF(!parameter.Descriptor.ptr);
				cmd->SetComputeRootDescriptorTable(parameter.RPIndex, parameter.Descriptor);
			}
		}
	}

	void Shader::SetGraphicsRootParameters(ID3D12GraphicsCommandList* cmd)
	{
		for (const auto& [hash, parameter] : ParameterMap)
		{
			if (parameter.Type == ParameterType::Constants)
			{
				if (!parameter.Data)
				{
					LB_WARN("'%s': Shader root parameter at index %d of ParameterType::%s has no value!", m_Name.data(), parameter.RPIndex, ParameterTypeToStr(parameter.Type).data());
					continue;
				}

				cmd->SetGraphicsRoot32BitConstants(parameter.RPIndex, parameter.NumValues, parameter.Data, parameter.Offset);
			}
			else
			{
				if (!parameter.Descriptor.ptr)
				{
					LB_WARN("'%s': Shader root parameter at index %d of ParameterType::%s has no value!", m_Name.data(), parameter.RPIndex, ParameterTypeToStr(parameter.Type).data());
					continue;
				}

				cmd->SetGraphicsRootDescriptorTable(parameter.RPIndex, parameter.Descriptor);
			}
		}
	}

	void Shader::SetConstant(const char* parameterName, const void* data)
	{
		uint32 hash = Algo::Hash(parameterName);
		if (!ParameterMap.contains(hash))
		{
			LB_WARN("'%s': Tried to set constant '%s' but couldn't it be found", m_Name.c_str(), parameterName);
			return;
		}
		ParameterMap[hash].Data = data;
	}

	void Shader::SetTexture(const char* parameterName, Handle<Texture> texture, uint32 mipLevel)
	{
		uint32 level = mipLevel == ~0 ? 0 : mipLevel;
		uint32 hash = Algo::Hash(parameterName);
		if (!ParameterMap.contains(hash))
		{
			LB_WARN("Tried to set parameter '%s' but the parameter was not found in the shader", parameterName);
			return;
		}
		ParameterInfo& parameter = ParameterMap[hash];

		Texture* t = ResourceManager::Ptr->GetTexture(texture);
		FAILIF(!t);

		D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_COMMON;
		if (parameter.Type == ParameterType::UAV)
		{
			FAILIF(t->BasicHandle[level].GPUHandle.ptr == 0); // texture is not a SRV
			newState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			parameter.Descriptor = t->BasicHandle[level].GPUHandle;
		}
		else if (parameter.Type == ParameterType::SRV)
		{
			FAILIF(t->SRVHandle[level].GPUHandle.ptr == 0); // texture is not a SRVs
			newState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			parameter.Descriptor = t->SRVHandle[level].GPUHandle;
		}
		else
		{
			ensure(false);
		}

		Device::Ptr->TransitionResource(t, newState, mipLevel);
	}

	void Shader::SetBuffer(const char* parameterName, Handle<Buffer> buffer)
	{
		uint32 hash = Algo::Hash(parameterName);
		if (!ParameterMap.contains(hash))
		{
			LB_WARN("Tried to set parameter '%s' but the parameter was not found in the shader", parameterName);
			return;
		}
		ParameterInfo& parameter = ParameterMap[hash];

		Buffer* b = ResourceManager::Ptr->GetBuffer(buffer);
		FAILIF(!b);

		D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_COMMON;
		if (parameter.Type == ParameterType::UAV)
			newState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		else if (parameter.Type == ParameterType::SRV)
			newState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		else if (parameter.Type == ParameterType::CBV)
			newState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		else
			ensure(false);

		Device::Ptr->TransitionResource(b, newState);

		parameter.Descriptor = b->BasicHandle.GPUHandle;
	}

	void Shader::SetSampler(const char* parameterName, Handle<Sampler> sampler)
	{
		uint32 hash = Algo::Hash(parameterName);
		if (!ParameterMap.contains(hash))
		{
			LB_WARN("Tried to set parameter '%s' but the parameter was not found in the shader", parameterName);
			return;
		}
		ParameterInfo& parameter = ParameterMap[hash];

		Sampler* s = ResourceManager::Ptr->GetSampler(sampler);
		FAILIF(!s);

		parameter.Descriptor = s->Handle.GPUHandle;
	}

	void Shader::CreateRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags, SC::Kernel const* kernels, uint32 kernelsCount)
	{
		ParameterMap.clear();

		CD3DX12_ROOT_PARAMETER1   rootParameters[64] = {}; // root signature limits https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits#memory-limits-and-costs
		CD3DX12_DESCRIPTOR_RANGE1 ranges[64] = {};
		uint32 currentRP = 0;
		uint32 rsCost    = 0;

		auto processResourceBinding = [&rootParameters, &ranges, &currentRP, &rsCost, this](const D3D12_SHADER_INPUT_BIND_DESC& bindDesc, ID3D12ShaderReflection* reflection)
		{
			switch (bindDesc.Type) {
			case D3D_SIT_CBUFFER:
			{
				auto* cb = reflection->GetConstantBufferByName(bindDesc.Name);
				if (strcmp(bindDesc.Name, "$Globals") == 0)
				{
					D3D12_SHADER_BUFFER_DESC desc;
					cb->GetDesc(&desc);
					uint32 num32BitValues = desc.Size / sizeof(uint32);
					bool   bAddedVar = false;
					for (uint32 v = 0; v < desc.Variables; ++v)
					{
						auto* var = cb->GetVariableByIndex(v);
						D3D12_SHADER_VARIABLE_DESC varDesc;
						var->GetDesc(&varDesc);

						uint32 hashedName = Algo::Hash(varDesc.Name);
						if (ParameterMap.contains(hashedName)) continue;

						bAddedVar = true;

						uint32 numValues = varDesc.Size / sizeof(uint32);
						ParameterMap[hashedName] = {
							.Type = ParameterType::Constants,
							.RPIndex = currentRP,
							.NumValues = numValues,
							.Offset = varDesc.StartOffset / sizeof(uint32)
						};
					}

					if (bAddedVar)
					{
						rootParameters[currentRP].InitAsConstants(num32BitValues, bindDesc.BindPoint, bindDesc.Space);
						currentRP++;
						rsCost += desc.Variables;
					}

					break;
				}

				// Normal constant buffers
				ParameterMap[Algo::Hash(bindDesc.Name)] = {
					.Type = ParameterType::CBV,
					.RPIndex = currentRP,
				};

				ranges[currentRP].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, bindDesc.BindPoint, bindDesc.Space);

				rootParameters[currentRP].InitAsDescriptorTable(1, &ranges[currentRP]);

				currentRP++;
				rsCost += 1;

				break;
			}
			case D3D_SIT_UAV_RWBYTEADDRESS:
			case D3D_SIT_UAV_APPEND_STRUCTURED:
			case D3D_SIT_UAV_CONSUME_STRUCTURED:
			case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
			case D3D_SIT_UAV_RWSTRUCTURED:
			case D3D_SIT_UAV_FEEDBACKTEXTURE:
			case D3D_SIT_UAV_RWTYPED: 
			{
				ParameterMap[Algo::Hash(bindDesc.Name)] = {
					.Type = ParameterType::UAV,
					.RPIndex = currentRP,
				};

				ranges[currentRP].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, bindDesc.BindPoint, bindDesc.Space);

				rootParameters[currentRP].InitAsDescriptorTable(1, &ranges[currentRP]);

				currentRP++;
				rsCost += 1;

				break;
			}
			case D3D_SIT_STRUCTURED: ensure(false); break;
			case D3D_SIT_BYTEADDRESS: ensure(false); break;
			case D3D_SIT_RTACCELERATIONSTRUCTURE: ensure(false); break;
			case D3D_SIT_TBUFFER: ensure(false); break;
#if 1
			case D3D_SIT_TEXTURE:
			{
				ParameterMap[Algo::Hash(bindDesc.Name)] = {
					.Type = ParameterType::SRV,
					.RPIndex = currentRP,
				};

				ranges[currentRP].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bindDesc.BindPoint, bindDesc.Space);

				rootParameters[currentRP].InitAsDescriptorTable(1, &ranges[currentRP]);

				currentRP++;
				rsCost += 1;

				break;
			}
#endif
			case D3D_SIT_SAMPLER:
			{
				ParameterMap[Algo::Hash(bindDesc.Name)] = {
					.Type = ParameterType::Samplers,
					.RPIndex = currentRP,
				};

				ranges[currentRP].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, bindDesc.BindPoint, bindDesc.Space);

				rootParameters[currentRP].InitAsDescriptorTable(1, &ranges[currentRP]);

				currentRP++;
				rsCost += 1;
				
				break;
			}
			default: LB_ERROR("Invalid Resource Type");
			}
		};

		for (uint32 i = 0; i < kernelsCount; ++i)
		{
			ID3D12ShaderReflection* reflection = kernels[i].Reflection.Get();

			D3D12_SHADER_DESC shaderDesc;
			reflection->GetDesc(&shaderDesc);

			// Bindings
			for (uint32 r = 0; r < shaderDesc.BoundResources; ++r)
			{
				D3D12_SHADER_INPUT_BIND_DESC bindDesc;
				reflection->GetResourceBindingDesc(r, &bindDesc);

				processResourceBinding(bindDesc, reflection);
			}
		}

		ensure(rsCost <= 64);

		flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Init_1_1(currentRP, rootParameters, 0, nullptr, flags);

		ID3DBlob* rootSigBlob;
		ID3DBlob* errorBlob;
		D3D12SerializeVersionedRootSignature(&desc, &rootSigBlob, &errorBlob);
		if (errorBlob)
			LB_ERROR("%s", (char*)errorBlob->GetBufferPointer());

		DX_CHECK(device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));

		std::wstring wname;
		Utils::StringConvert(m_Name, wname);
		wname.append(L" RS");
		DX_CHECK(RootSignature->SetName(wname.c_str()));
	}

	void Shader::CreateComputePipeline(ID3D12Device* device, const ShaderSpec& spec)
	{
		FAILIF(!spec.ProgramName);

		SC::Kernel cs;
		FAILIF(!SC::Compile(cs, spec.ProgramName, spec.CsEntryPoint, SC::KernelType::Compute));

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		CreateRootSignature(device, rootSignatureFlags, &cs, 1);

		D3D12_SHADER_BYTECODE shaderByteCode = {
			.pShaderBytecode = cs.Bytecode->GetBufferPointer(),
			.BytecodeLength = cs.Bytecode->GetBufferSize()
		};

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			.pRootSignature = RootSignature.Get(),
			.CS = shaderByteCode,
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
		DX_CHECK(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&PipelineState)));
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
				.Width = Device::Ptr->GetBackbufferWidth(),
				.Height = Device::Ptr->GetBackbufferHeight(),
				.DebugName = debugName.c_str(),
				.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
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
		for (uint8 i = 0; spec.RTFormats[i].RTFormat != Format::UNKNOWN || spec.RTFormats[i].RTTexture.IsValid(); ++i)
			RTCount++;
		if (RTCount <= 0)
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

			if (spec.DepthFormat.RTFormat == Format::UNKNOWN && !spec.DepthFormat.RTTexture.IsValid())
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
							.Width = Device::Ptr->GetBackbufferWidth(),
							.Height = Device::Ptr->GetBackbufferHeight(),
							.DebugName = debugName.c_str(),
							.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
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

		InputLayout inputLayout;
		CreateInputLayout(vs, inputLayout);

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		SC::Kernel kernels[2] = { vs, ps };
		CreateRootSignature(device, rootSignatureFlags, kernels, 2);

		D3D12_SHADER_BYTECODE vsBytecode = {
			.pShaderBytecode = vs.Bytecode->GetBufferPointer(),
			.BytecodeLength = vs.Bytecode->GetBufferSize()
		};

		D3D12_SHADER_BYTECODE psBytecode = {
			.pShaderBytecode = ps.Bytecode->GetBufferPointer(),
			.BytecodeLength = ps.Bytecode->GetBufferSize()
		};

		D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = true;

		D3D12_BLEND_DESC blendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		blendState.RenderTarget[0] = GetDefaultEnabledBlendDesc();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
			.pRootSignature = RootSignature.Get(),
			.VS = vsBytecode,
			.PS = psBytecode,
			.StreamOutput = nullptr,
			.BlendState = blendState,
			.SampleMask = 0xFFFFFFFF,
			.RasterizerState = rasterizerDesc,
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.InputLayout = {
				inputLayout.data(),
				(uint32)inputLayout.size(),
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
			if (RTCount <= 0)
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

				if (spec.DepthFormat.RTFormat == Format::UNKNOWN && !spec.DepthFormat.RTTexture.IsValid())
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

	void Shader::CreateInputLayout(const SC::Kernel& vs, InputLayout& outInputLayout)
	{
		ID3D12ShaderReflection* reflection = vs.Reflection.Get();

		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);

		// Input layout
		outInputLayout.resize(shaderDesc.InputParameters);
		for (uint32 i = 0; i < shaderDesc.InputParameters; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC desc;
			reflection->GetInputParameterDesc(i, &desc);

			uint32 componentCount = 0;
			while (desc.Mask)
			{
				++componentCount;
				desc.Mask >>= 1;
			}

			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			switch (desc.ComponentType)
			{
			case D3D_REGISTER_COMPONENT_UINT32:
				format = (componentCount == 1 ? DXGI_FORMAT_R32_UINT :
					componentCount == 2 ? DXGI_FORMAT_R32G32_UINT :
					componentCount == 3 ? DXGI_FORMAT_R32G32B32_UINT :
					DXGI_FORMAT_R32G32B32A32_UINT);
				break;
			case D3D_REGISTER_COMPONENT_SINT32:
				format = (componentCount == 1 ? DXGI_FORMAT_R32_SINT :
					componentCount == 2 ? DXGI_FORMAT_R32G32_SINT :
					componentCount == 3 ? DXGI_FORMAT_R32G32B32_SINT :
					DXGI_FORMAT_R32G32B32A32_SINT);
				break;
			case D3D_REGISTER_COMPONENT_FLOAT32:
				format = (componentCount == 1 ? DXGI_FORMAT_R32_FLOAT :
					componentCount == 2 ? DXGI_FORMAT_R32G32_FLOAT :
					componentCount == 3 ? DXGI_FORMAT_R32G32B32_FLOAT :
					DXGI_FORMAT_R32G32B32A32_FLOAT);
				break;
			default:
				break;  // D3D_REGISTER_COMPONENT_UNKNOWN
			}

			outInputLayout[i] = {
				.SemanticName = desc.SemanticName,
				.SemanticIndex = desc.SemanticIndex,
				.Format = format,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			};
		}
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
		RootSignature.Reset();

		if (Type == ShaderType::Compute)
			CreateComputePipeline(Device::Ptr->GetDevice(), m_Spec);
		else if (Type == ShaderType::Graphics)
			CreateGraphicsPipeline(Device::Ptr->GetDevice(), m_Spec, true);
	}

	std::string_view Shader::ParameterTypeToStr(ParameterType type)
	{
		switch (type)
		{
		case ParameterType::SRV: return "SRV";
		case ParameterType::UAV: return "UAV";
		case ParameterType::CBV: return "CBV";
		case ParameterType::Samplers: return "Samplers";
		case ParameterType::Constants: return "Constants";
		case ParameterType::MAX:
		default: return "";
		}
	}
}
