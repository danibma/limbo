#pragma once

#include "shader.h"
#include "d3d12definitions.h"

namespace limbo::rhi
{
	class D3D12Shader final : public Shader
	{
	public:
		ComPtr<ID3D12PipelineState>			pipelineState;

	public:
		D3D12Shader() = default;
		D3D12Shader(const ShaderSpec& spec);

		virtual ~D3D12Shader();

	private:
		void createComputePipeline(ID3D12Device* device, const ShaderSpec& spec);
		void createGraphicsPipeline(ID3D12Device* device);
	};
}