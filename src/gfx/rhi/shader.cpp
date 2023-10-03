#include "stdafx.h"
#include "shader.h"

namespace limbo::RHI
{
	Shader::Shader(const char* file, const char* entryPoint, ShaderType type)
		: File(file), EntryPoint(entryPoint), Type(type)
	{
	}
}
