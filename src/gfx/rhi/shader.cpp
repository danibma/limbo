#include "shader.h"
#include "device.h"
#include "resourcemanager.h"

#include "shadercompiler.h"

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
			createGraphicsPipeline(d3ddevice);
	}

	Shader::~Shader()
	{
	}

	void Shader::createComputePipeline(ID3D12Device* device, const ShaderSpec& spec)
	{
		ShaderResult shader;
		ShaderCompiler::compile(shader, spec.programName, spec.entryPoint, KernelType::Compute, false);

		D3D12_SHADER_BYTECODE shaderByteCode = {
			.pShaderBytecode = shader.code,
			.BytecodeLength = shader.size
		};

		ensure(spec.bindGroups.size() <= 1); // not sure why

		ResourceManager* rm = ResourceManager::ptr;
		BindGroup* bind = rm->getBindGroup(spec.bindGroups[0]);

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			.pRootSignature = bind->rootSignature.Get(),
			.CS = shaderByteCode,
			.NodeMask = 0,
			.CachedPSO = nullptr,
			.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};
		DX_CHECK(device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState)));
	}

	void Shader::createGraphicsPipeline(ID3D12Device* device)
	{
		ensure(false);
	}
}
