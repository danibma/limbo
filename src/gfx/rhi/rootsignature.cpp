#include "stdafx.h"
#include "rootsignature.h"
#include "shadercompiler.h"

#include <d3d12shader.h>

namespace limbo::RHI
{
	RootSignature::RootSignature(const std::string& name)
		: m_Name(name)
	{
	}

	void RootSignature::CreateRootSignature(ID3D12Device* device, D3D12_ROOT_SIGNATURE_FLAGS flags, SC::Kernel* kernels, uint32 kernelsCount)
	{
		m_ParameterMap.clear();

		CD3DX12_ROOT_PARAMETER1   rootParameters[64] = {}; // root signature limits https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits#memory-limits-and-costs
		CD3DX12_DESCRIPTOR_RANGE1 ranges[64] = {};
		uint32 currentRP = 0;
		uint32 rsCost = 0;

		D3D12_STATIC_SAMPLER_DESC staticSamplers[16] = {};
		auto addStaticSampler = [&staticSamplers](uint32 registerSlot, D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE wrapMode, D3D12_COMPARISON_FUNC compareFunc = D3D12_COMPARISON_FUNC_ALWAYS)
		{
			staticSamplers[registerSlot] = {
				.Filter = filter,
				.AddressU = wrapMode,
				.AddressV = wrapMode,
				.AddressW = wrapMode,
				.MipLODBias = 0.0f,
				.MaxAnisotropy = 8,
				.ComparisonFunc = compareFunc,
				.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
				.MinLOD = 0.0f,
				.MaxLOD = FLT_MAX,
				.ShaderRegister = registerSlot,
				.RegisterSpace = 1,
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
			};
		};

		auto processResourceBinding = [&rootParameters, &ranges, &currentRP, &rsCost, this](const D3D12_SHADER_INPUT_BIND_DESC& bindDesc, auto* reflection)
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
						if (m_ParameterMap.contains(hashedName)) continue;

						bAddedVar = true;

						uint32 numValues = varDesc.Size / sizeof(uint32);
						m_ParameterMap[hashedName] = {
							.Type = ShaderParameterType::Constants,
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
				uint32 hashedName = Algo::Hash(bindDesc.Name);
				if (m_ParameterMap.contains(hashedName)) break;

				m_ParameterMap[hashedName] = {
					.Type = ShaderParameterType::CBV,
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
				uint32 hashedName = Algo::Hash(bindDesc.Name);
				if (m_ParameterMap.contains(hashedName)) break;

				m_ParameterMap[hashedName] = {
					.Type = ShaderParameterType::UAV,
					.RPIndex = currentRP,
				};

				ranges[currentRP].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, bindDesc.BindPoint, bindDesc.Space);

				rootParameters[currentRP].InitAsDescriptorTable(1, &ranges[currentRP]);

				currentRP++;
				rsCost += 1;

				break;
			}
			case D3D_SIT_BYTEADDRESS: ensure(false); break;
			case D3D_SIT_TBUFFER: ensure(false); break;
			case D3D_SIT_STRUCTURED:
			case D3D_SIT_RTACCELERATIONSTRUCTURE:
			case D3D_SIT_TEXTURE:
			{
				uint32 hashedName = Algo::Hash(bindDesc.Name);
				if (m_ParameterMap.contains(hashedName)) break;

				m_ParameterMap[hashedName] = {
					.Type = ShaderParameterType::SRV,
					.RPIndex = currentRP,
				};

				ranges[currentRP].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bindDesc.BindPoint, bindDesc.Space);

				rootParameters[currentRP].InitAsDescriptorTable(1, &ranges[currentRP]);

				currentRP++;
				rsCost += 1;

				break;
			}
			case D3D_SIT_SAMPLER: // Don't do anything, we use static samplers
				break;
			default: LB_ERROR("Invalid Resource Type");
			}
		};

		for (uint32 i = 0; i < kernelsCount; ++i)
		{
			if (ID3D12LibraryReflection* libReflection = kernels[i].LibReflection.Get())
			{
				D3D12_LIBRARY_DESC libDesc;
				libReflection->GetDesc(&libDesc);

				for (uint32 f = 0; f < libDesc.FunctionCount; f++)
				{
					ID3D12FunctionReflection* func = libReflection->GetFunctionByIndex(f);
					D3D12_FUNCTION_DESC funcDesc;
					func->GetDesc(&funcDesc);

					// Bindings
					for (uint32 r = 0; r < funcDesc.BoundResources; ++r)
					{
						D3D12_SHADER_INPUT_BIND_DESC bindDesc;
						func->GetResourceBindingDesc(r, &bindDesc);

						processResourceBinding(bindDesc, func);
					}
				}
			}
			else if (ID3D12ShaderReflection* reflection = kernels[i].Reflection.Get())
			{
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
		}

		ensure(rsCost <= 64);

		flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		// Static Samplers: these are the equivalents of the samplers in bindings.hlsl
		uint32 numStaticSamplers = 0;
		addStaticSampler(numStaticSamplers++, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP);   // SLinearWrap
		addStaticSampler(numStaticSamplers++, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);  // SLinearClamp
		addStaticSampler(numStaticSamplers++, D3D12_FILTER_MIN_MAG_MIP_POINT,  D3D12_TEXTURE_ADDRESS_MODE_WRAP);   // SPointWrap

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
		desc.Init_1_1(currentRP, rootParameters, numStaticSamplers, staticSamplers, flags);

		ID3DBlob* rootSigBlob;
		ID3DBlob* errorBlob;
		D3D12SerializeVersionedRootSignature(&desc, &rootSigBlob, &errorBlob);
		if (errorBlob)
			LB_ERROR("%s", (char*)errorBlob->GetBufferPointer());

		DX_CHECK(device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&m_RS)));

		std::wstring wname;
		Utils::StringConvert(m_Name, wname);
		DX_CHECK(m_RS->SetName(wname.c_str()));
	}

	void RootSignature::SetRootParameters(ShaderType shaderType, ID3D12GraphicsCommandList* cmd)
	{
		for (const auto& [hash, parameter] : m_ParameterMap)
		{
			if (parameter.Type == ShaderParameterType::Constants)
			{
				if (!parameter.Data)
				{
					LB_WARN("'%s': Shader root parameter at index %d of ParameterType::%s has no value!", m_Name.data(), parameter.RPIndex, ShaderParameterTypeToStr(parameter.Type).data());
					continue;
				}

				if (shaderType == ShaderType::Graphics)
					cmd->SetGraphicsRoot32BitConstants(parameter.RPIndex, parameter.NumValues, parameter.Data, parameter.Offset);
				else // Compute and Ray Tracing
					cmd->SetComputeRoot32BitConstants(parameter.RPIndex, parameter.NumValues, parameter.Data, parameter.Offset);
			}
			else
			{
				if (!parameter.Descriptor.ptr)
				{
					LB_WARN("'%s': Shader root parameter at index %d of ParameterType::%s has no value!", m_Name.data(), parameter.RPIndex, ShaderParameterTypeToStr(parameter.Type).data());
					continue;
				}

				if (shaderType == ShaderType::Graphics)
					cmd->SetGraphicsRootDescriptorTable(parameter.RPIndex, parameter.Descriptor);
				else // Compute and Ray Tracing
					cmd->SetComputeRootDescriptorTable(parameter.RPIndex, parameter.Descriptor);
			}
		}
	}

	ShaderParameterInfo& RootSignature::GetParameter(const char* parameterName)
	{
		static ShaderParameterInfo InvalidShaderParameter = { ShaderParameterType::MAX };

		uint32 hash = Algo::Hash(parameterName);
		if (!m_ParameterMap.contains(hash))
		{
			LB_WARN("'%s': Tried to set parameter '%s' but the parameter was not found in the shader", m_Name.c_str(), parameterName);
			return InvalidShaderParameter;
		}
		return m_ParameterMap[hash];
	}

	void RootSignature::Reset()
	{
		m_RS.Reset();
	}
}
