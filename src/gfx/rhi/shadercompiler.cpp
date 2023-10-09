#include "stdafx.h"
#include "shadercompiler.h"
#include "core/utils.h"
#include "resourcemanager.h"

#include <dxcapi.h>

namespace limbo::RHI::SC
{
	bool Compile(ShaderHandle shader)
	{
		return Compile(RM_GET(shader));
	}

	bool Compile(Shader* pShader)
	{
		static IDxcUtils* dxcUtils;
		static IDxcCompiler3* dxcCompiler;
		static IDxcIncludeHandler* includeHandler;
		if (!dxcCompiler)
		{
			HMODULE lib = LoadLibraryA("dxcompiler.dll");
			DxcCreateInstanceProc DxcCreateInstanceFn = (DxcCreateInstanceProc)GetProcAddress(lib, "DxcCreateInstance");

			DxcCreateInstanceFn(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
			DxcCreateInstanceFn(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
			dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
		}


		// setup arguments
		std::string cpath = std::string("shaders/" + std::string(pShader->File));
		std::wstring path;
		Utils::StringConvert(cpath, path);

		std::wstring profile = L"-T ";
		switch (pShader->Type)
		{
		case ShaderType::Compute:
			profile.append(L"cs");
			break;
		case ShaderType::Vertex:
			profile.append(L"vs");
			break;
		case ShaderType::Pixel:
			profile.append(L"ps");
			break;
		case ShaderType::Lib:
			profile.append(L"lib");
		}
		profile.append(L"_6_6");


		std::vector<const wchar_t*> arguments;
		arguments.reserve(20);
		arguments.emplace_back(path.c_str());

		std::wstring wentrypoint;
		if (pShader->Type != ShaderType::Lib)
		{
			Utils::StringConvert(pShader->EntryPoint, wentrypoint);
			wentrypoint.insert(0, L"-E ");
			arguments.emplace_back(wentrypoint.c_str());
		}

		arguments.emplace_back(profile.c_str());

#if !LIMBO_DEBUG
		if (Core::CommandLine::HasArg(LIMBO_CMD_PROFILE))
#endif
		{
			arguments.emplace_back(DXC_ARG_SKIP_OPTIMIZATIONS); // Disable optimizations
			arguments.emplace_back(DXC_ARG_DEBUG);              // Enable debug information
			arguments.emplace_back(L"-Qembed_debug");           // Embed PDB in shader container (must be used with /Zi)
		}

		arguments.emplace_back(DXC_ARG_ALL_RESOURCES_BOUND);

		// Compile shader
		RefCountPtr<IDxcBlobEncoding> source = nullptr;
		dxcUtils->LoadFile(path.c_str(), nullptr, source.ReleaseAndGetAddressOf());
		if (!source)
		{
			LB_ERROR("Could not find the program shader file: %s", cpath.c_str());
			return false;
		}

		DxcBuffer sourceBuffer;
		sourceBuffer.Ptr = source->GetBufferPointer();
		sourceBuffer.Size = source->GetBufferSize();
		sourceBuffer.Encoding = DXC_CP_ACP;

		RefCountPtr<IDxcResult> compileResult;
		DX_CHECK(dxcCompiler->Compile(&sourceBuffer, arguments.data(), (uint32)arguments.size(), includeHandler, IID_PPV_ARGS(compileResult.ReleaseAndGetAddressOf())));

		// Print errors if present.
		RefCountPtr<IDxcBlobUtf8> errors = nullptr;
		DX_CHECK(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()), nullptr));
		// Note that d3dcompiler would return null if no errors or warnings are present.  
		// IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
		if (errors != nullptr && errors->GetStringLength() != 0)
		{
			LB_ERROR("%s", errors->GetStringPointer());
			return false;
		}

		// Get shader blob
		DX_CHECK(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader->Bytecode.ReleaseAndGetAddressOf()), nullptr));

		if (!pShader->IsCompiled())
			pShader->AddToRecompilation();

		return true;
	}

}
