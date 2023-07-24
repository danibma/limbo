#include "shader.h"

#include <dxcapi.h>

#include "device.h"
#include "shadercompiler.h"

#include "core/algo.h"

#include <d3d12/d3dx12/d3dx12.h>

#include "resourcemanager.h"

namespace limbo::gfx
{
	Shader::Shader(const ShaderSpec& spec)
	{
		Device* device = Device::ptr;
		ID3D12Device* d3ddevice = device->getDevice();

		type = spec.type;

		if (spec.type == ShaderType::Compute)
			createComputePipeline(d3ddevice, spec);
		else if (spec.type == ShaderType::Graphics)
			createGraphicsPipeline(d3ddevice, spec);
	}

	Shader::~Shader()
	{
	}

	void Shader::setComputeRootParameters(ID3D12GraphicsCommandList* cmd)
	{
		for (const auto& [name, parameter] : parameterMap)
		{
			if (parameter.type == ParameterType::Constants)
			{
				FAILIF(!parameter.data);
				cmd->SetComputeRoot32BitConstants(parameter.rpIndex, parameter.numValues, parameter.data, parameter.offset);
			}
			else
			{
				FAILIF(!parameter.descriptor.ptr);
				cmd->SetComputeRootDescriptorTable(parameter.rpIndex, parameter.descriptor);
			}
		}
	}

	void Shader::setGraphicsRootParameters(ID3D12GraphicsCommandList* cmd)
	{
		for (const auto& [name, parameter] : parameterMap)
		{
			if (parameter.type == ParameterType::Constants)
			{
				FAILIF(!parameter.data);
				cmd->SetGraphicsRoot32BitConstants(parameter.rpIndex, parameter.numValues, parameter.data, parameter.offset);
			}
			else
			{
				FAILIF(!parameter.descriptor.ptr);
				cmd->SetGraphicsRootDescriptorTable(parameter.rpIndex, parameter.descriptor);
			}
		}
	}

	void Shader::setConstant(const char* parameterName, const void* data)
	{
		uint32 hash = algo::hash(parameterName);
		FAILIF(!parameterMap.contains(hash));
		parameterMap[hash].data = data;
	}

	void Shader::setTexture(const char* parameterName, Handle<Texture> texture)
	{
		uint32 hash = algo::hash(parameterName);
		FAILIF(!parameterMap.contains(hash));
		ParameterInfo& parameter = parameterMap[hash];

		Texture* t = ResourceManager::ptr->getTexture(texture);
		FAILIF(!t);

		D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_COMMON;
		if (parameter.type == ParameterType::UAV)
			newState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		else if (parameter.type == ParameterType::SRV)
			newState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		else
			ensure(false);

		Device::ptr->transitionResource(t, newState);

		parameter.descriptor = t->handle.gpuHandle;
	}

	void Shader::setBuffer(const char* parameterName, Handle<Buffer> buffer)
	{
		uint32 hash = algo::hash(parameterName);
		FAILIF(!parameterMap.contains(hash));
		ParameterInfo& parameter = parameterMap[hash];

		Buffer* b = ResourceManager::ptr->getBuffer(buffer);
		FAILIF(!b);

		D3D12_RESOURCE_STATES newState = D3D12_RESOURCE_STATE_COMMON;
		if (parameter.type == ParameterType::UAV)
			newState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		else if (parameter.type == ParameterType::SRV)
			newState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		else
			ensure(false);

		Device::ptr->transitionResource(b, newState);

		parameter.descriptor = b->handle.gpuHandle;
	}

	void Shader::setSampler(const char* parameterName, Handle<Sampler> sampler)
	{
		uint32 hash = algo::hash(parameterName);
		FAILIF(!parameterMap.contains(hash));
		ParameterInfo& parameter = parameterMap[hash];

		Sampler* s = ResourceManager::ptr->getSampler(sampler);
		FAILIF(!s);

		parameter.descriptor = s->handle.gpuHandle;
	}
	void Shader::createRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags, SC::Kernel const* kernels, uint32 kernelsCount)
	{
		CD3DX12_ROOT_PARAMETER1 rootParameters[64]; // root signature limits https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits#memory-limits-and-costs
		uint32 currentRP = 0;
		uint32 rsCost    = 0;

		auto processResourceBinding = [&rootParameters, &currentRP, &rsCost, this](const D3D12_SHADER_INPUT_BIND_DESC& bindDesc, ID3D12ShaderReflection* reflection)
		{
			switch (bindDesc.Type) {
			case D3D_SIT_CBUFFER:
			{
				auto* cb = reflection->GetConstantBufferByName(bindDesc.Name);
				if (strcmp(bindDesc.Name, "$Globals") == 0)
				{
					D3D12_SHADER_BUFFER_DESC desc;
					cb->GetDesc(&desc);
					uint32 num32BitValues = 0;
					bool   bAddedVar = false;
					for (uint32 v = 0; v < desc.Variables; ++v)
					{
						auto* var = cb->GetVariableByIndex(v);
						D3D12_SHADER_VARIABLE_DESC varDesc;
						var->GetDesc(&varDesc);

						uint32 hashedName = algo::hash(varDesc.Name);
						if (parameterMap.contains(hashedName)) continue;

						bAddedVar = true;

						uint32 numValues = varDesc.Size / sizeof(uint32);
						parameterMap[hashedName] = {
							.type = ParameterType::Constants,
							.rpIndex = currentRP,
							.numValues = numValues,
							.offset = varDesc.StartOffset / sizeof(uint32)
						};
						
						num32BitValues += numValues;
					}

					if (bAddedVar)
					{
						rootParameters[currentRP].InitAsConstants(num32BitValues, bindDesc.BindPoint, bindDesc.Space);
						currentRP++;
						rsCost += desc.Variables;
					}

					break;
				}
				ensure(false); // why use cbuffers instead of 32bit constants?
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
				parameterMap[algo::hash(bindDesc.Name)] = {
					.type = ParameterType::UAV,
					.rpIndex = currentRP,
				};

				CD3DX12_DESCRIPTOR_RANGE1 range;
				range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, bindDesc.BindPoint, bindDesc.Space);

				rootParameters[currentRP].InitAsDescriptorTable(1, &range);

				currentRP++;
				rsCost += 1;

				break;
			}
			case D3D_SIT_STRUCTURED: ensure(false); break;
			case D3D_SIT_BYTEADDRESS: ensure(false); break;
			case D3D_SIT_RTACCELERATIONSTRUCTURE: ensure(false); break;
			case D3D_SIT_TBUFFER: ensure(false); break;
			case D3D_SIT_TEXTURE: ensure(false); break;
			case D3D_SIT_SAMPLER: ensure(false); break;
			default: LB_ERROR("Invalid Resource Type");
			}
		};

		for (uint32 i = 0; i < kernelsCount; ++i)
		{
			ID3D12ShaderReflection* reflection = kernels[i].reflection.Get();

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

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Init_1_1(currentRP, rootParameters, 0, nullptr, flags);

		ID3DBlob* rootSigBlob;
		ID3DBlob* errorBlob;
		D3D12SerializeVersionedRootSignature(&desc, &rootSigBlob, &errorBlob);
		if (errorBlob)
			LB_ERROR("%s", (char*)errorBlob->GetBufferPointer());

		DX_CHECK(device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
	}

	void Shader::createComputePipeline(ID3D12Device* device, const ShaderSpec& spec)
	{
		FAILIF(!spec.programName);

		SC::Kernel cs;
		FAILIF(!SC::compile(cs, spec.programName, spec.cs_entryPoint, SC::KernelType::Compute));

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

		createRootSignature(device, rootSignatureFlags, &cs, 1);

		D3D12_SHADER_BYTECODE shaderByteCode = {
			.pShaderBytecode = cs.bytecode->GetBufferPointer(),
			.BytecodeLength = cs.bytecode->GetBufferSize()
		};

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			.pRootSignature = rootSignature.Get(),
			.CS = shaderByteCode,
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
		DX_CHECK(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState)));
	}

	void Shader::createGraphicsPipeline(ID3D12Device* device, const ShaderSpec& spec)
	{
		FAILIF(spec.rtCount > 8)
		FAILIF(!spec.programName);

		SC::Kernel vs;
		FAILIF(!SC::compile(vs, spec.programName, spec.vs_entryPoint, SC::KernelType::Vertex));
		SC::Kernel ps;
		FAILIF(!SC::compile(ps, spec.programName, spec.ps_entryPoint, SC::KernelType::Pixel));

		InputLayout inputLayout;
		createInputLayout(vs, inputLayout);

		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		SC::Kernel kernels[2] = { vs, ps };
		createRootSignature(device, rootSignatureFlags, kernels, 2);

		D3D12_SHADER_BYTECODE vsBytecode = {
			.pShaderBytecode = vs.bytecode->GetBufferPointer(),
			.BytecodeLength = vs.bytecode->GetBufferSize()
		};

		D3D12_SHADER_BYTECODE psBytecode = {
			.pShaderBytecode = ps.bytecode->GetBufferPointer(),
			.BytecodeLength = ps.bytecode->GetBufferSize()
		};

		D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		rasterizerDesc.FrontCounterClockwise = false;

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
			.pRootSignature = rootSignature.Get(),
			.VS = vsBytecode,
			.PS = psBytecode,
			.StreamOutput = nullptr,
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = 0xFFFFFFFF,
			.RasterizerState = rasterizerDesc,
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.InputLayout = {
				inputLayout.data(),
				(uint32)inputLayout.size(),
			},
			.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
			.PrimitiveTopologyType = spec.topology,
			.DSVFormat = d3dFormat(spec.depthFormat),
			.SampleDesc = {
				.Count = 1,
				.Quality = 0 
			},
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};

		if (spec.depthFormat == Format::MAX)
		{
			desc.DepthStencilState.DepthEnable	 = false;
			desc.DepthStencilState.StencilEnable = false;
		}

		if (spec.rtCount <= 0)
		{
			desc.RTVFormats[0]		= d3dFormat(Device::ptr->getSwapchainFormat());
			desc.NumRenderTargets	= 1;
			useSwapchainRT			= true;
		}
		else
		{
			useSwapchainRT = false;

			for (uint8 i = 0; i < spec.rtCount; ++i)
				desc.RTVFormats[i] = d3dFormat(spec.rtFormats[i]);
			desc.NumRenderTargets = spec.rtCount;
		}

		DX_CHECK(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState)));
	}

	void Shader::createInputLayout(const SC::Kernel& vs, InputLayout& outInputLayout)
	{
		ID3D12ShaderReflection* reflection = vs.reflection.Get();

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
}
