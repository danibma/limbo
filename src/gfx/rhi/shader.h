#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "shadercompiler.h"

namespace limbo::gfx
{
	class  BindGroup;
	struct ShaderSpec
	{
		Kernel vs;
		Kernel ps;
		Kernel cs;

		uint8  rtCount;
		Format rtFormats[8];
		Format depthFormat = Format::MAX;

		D3D12_PRIMITIVE_TOPOLOGY_TYPE	topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		Handle<BindGroup>				bindGroup;
		ShaderType						type = ShaderType::Graphics;
	};

	class Shader
	{
	public:
		ShaderType							type;
		ComPtr<ID3D12PipelineState>			pipelineState;

	public:
		Shader() = default;
		Shader(const ShaderSpec& spec);

		~Shader();

	private:
		void createComputePipeline(ID3D12Device* device, const ShaderSpec& spec);
		void createGraphicsPipeline(ID3D12Device* device, const ShaderSpec& spec);
	};
}
