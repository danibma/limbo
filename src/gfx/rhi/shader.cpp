#include "stdafx.h"
#include "shader.h"
#include "shadercompiler.h"

namespace limbo::RHI
{
	Shader::Shader(const char* file, const char* entryPoint, ShaderType type)
		: File(file), EntryPoint(entryPoint), Type(type)
	{
	}

	Shader::~Shader()
	{
		OnReloadShaders.Remove(m_RecompilationHandle);
	}

	void Shader::AddToRecompilation()
	{
		m_RecompilationHandle = OnReloadShaders.AddLambda([this]()
		{
			SC::Compile(this);
			LB_LOG("Recompiled %s sucessfully", File);
		});
	}
}
