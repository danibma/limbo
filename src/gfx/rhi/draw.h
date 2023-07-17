#pragma once
#include "resourcepool.h"

namespace limbo::gfx
{
	class Shader;
	class BindGroup;
	struct DrawInfo
	{
		Handle<Shader>	  shader;
		Handle<BindGroup> bindGroup;
	};
}
