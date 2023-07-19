#pragma once
#include "resourcepool.h"

namespace limbo::gfx
{
	class Shader;
	class BindGroup;
	struct DrawInfo
	{
		Handle<Shader>		shader;
		Handle<BindGroup>	bindGroup;

		D3D12_VIEWPORT		viewport;
		D3D12_RECT			scissor;
	};
}
