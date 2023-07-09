#include "d3d12shader.h"
#include "d3d12device.h"
#include "d3d12resourcemanager.h"

#include "shadercompiler.h"

namespace limbo::rhi
{
	D3D12Shader::D3D12Shader(const ShaderSpec& spec)
	{
		D3D12Device* device = Device::getAs<D3D12Device>();
		ID3D12Device* d3ddevice = device->getDevice();

		type = spec.type;

		if (spec.type == ShaderType::Compute)
			createComputePipeline(d3ddevice, spec);
		else if (spec.type == ShaderType::Graphics)
			createGraphicsPipeline(d3ddevice);
	}

	D3D12Shader::~D3D12Shader()
	{
	}

	void D3D12Shader::createComputePipeline(ID3D12Device* device, const ShaderSpec& spec)
	{
		ShaderResult shader;
		ShaderCompiler::compile(shader, spec.programName, spec.entryPoint, KernelType::Compute, false);

		D3D12_SHADER_BYTECODE shaderByteCode = {
			.pShaderBytecode = shader.code,
			.BytecodeLength = shader.size
		};

		ensure(spec.bindGroups.size() <= 1); // not sure why

		D3D12ResourceManager* rm = ResourceManager::getAs<D3D12ResourceManager>();
		D3D12BindGroup* bind = rm->getBindGroup(spec.bindGroups[0]);

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			.pRootSignature = bind->rootSignature.Get(),
			.CS = shaderByteCode,
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
		DX_CHECK(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState)));
	}

	void D3D12Shader::createGraphicsPipeline(ID3D12Device* device)
	{
		ensure(false);
	}
}
