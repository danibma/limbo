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
		~Shader();

		void AddToRecompilation();
		bool IsCompiled() const { return m_RecompilationHandle; }

	private:
		DelegateHandle m_RecompilationHandle;
	};
	typedef TypedHandle<Shader> ShaderHandle;
}
