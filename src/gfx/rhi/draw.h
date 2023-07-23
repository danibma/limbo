#pragma once
#include "resourcepool.h"

namespace limbo::gfx
{
	class Shader;
	struct DrawInfo
	{
		Handle<Shader>		shader;

		D3D12_VIEWPORT		viewport;
		D3D12_RECT			scissor;
	};
}
