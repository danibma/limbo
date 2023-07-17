#include "shader.h"
#include "device.h"
#include "resourcemanager.h"
#include "shadercompiler.h"

#include <d3d12/d3dx12/d3dx12.h>

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

	void Shader::createComputePipeline(ID3D12Device* device, const ShaderSpec& spec)
	{
		FAILIF(!spec.cs.code);
		FAILIF(spec.cs.size <= 0);

		D3D12_SHADER_BYTECODE shaderByteCode = {
			.pShaderBytecode = spec.cs.code,
			.BytecodeLength = spec.cs.size
		};

		ResourceManager* rm = ResourceManager::ptr;
		BindGroup* bind = rm->getBindGroup(spec.bindGroup);

		D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {
			.pRootSignature = bind->rootSignature.Get(),
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
		FAILIF(!spec.vs.code);
		FAILIF(spec.vs.size <= 0);
		FAILIF(!spec.ps.code);
		FAILIF(spec.ps.size <= 0);

		D3D12_SHADER_BYTECODE vs = {
			.pShaderBytecode = spec.vs.code,
			.BytecodeLength = spec.vs.size
		};

		D3D12_SHADER_BYTECODE ps = {
			.pShaderBytecode = spec.ps.code,
			.BytecodeLength = spec.ps.size
		};

		ResourceManager* rm = ResourceManager::ptr;
		BindGroup* bind = rm->getBindGroup(spec.bindGroup);

		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {
			.pRootSignature = bind->rootSignature.Get(),
			.VS = vs,
			.PS = ps,
			.StreamOutput = nullptr,
			.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
			.SampleMask = 0xFFFFFFFF,
			.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
			.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
			.InputLayout = {
				bind->inputElements.data(),
				(uint32)bind->inputElements.size(),
			},
			.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
			.PrimitiveTopologyType = spec.topology,
			.NumRenderTargets = spec.rtCount,
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

		for (uint8 i = 0; i < spec.rtCount; ++i)
			desc.RTVFormats[i] = d3dFormat(spec.rtFormats[i]);

		DX_CHECK(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState)));
	}
}
