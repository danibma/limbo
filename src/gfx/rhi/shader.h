#pragma once

#include "definitions.h"
#include "resourcepool.h"

#include <vector>

namespace limbo::gfx
{
	class BindGroup;
	struct ShaderSpec
	{
		const char* programName;
		const char* entryPoint;
		std::vector<Handle<BindGroup>>	bindGroups;
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
		void createGraphicsPipeline(ID3D12Device* device);
	};
}
