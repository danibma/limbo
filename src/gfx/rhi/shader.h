#pragma once

#include "definitions.h"
#include "resourcepool.h"
#include "core/refcountptr.h"

#include <dxcapi.h>
#include <CppDelegates/Delegates.h>

namespace limbo::RHI
{
	typedef RefCountPtr<IDxcBlob> ShaderBytecode;

	struct Shader
	{
		const char*		File;
		const char*		EntryPoint;
		ShaderType		Type;
		ShaderBytecode	Bytecode;

		Shader() = default;
		Shader(const char* file, const char* entryPoint, ShaderType type);
	};
	typedef Handle<Shader> ShaderHandle;
}
