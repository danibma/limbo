#pragma once

namespace limbo
{
	struct DrawInfo
	{
		Handle<Shader> shader;
		std::vector<Handle<BindGroup>> bindGroups;
	};
}
