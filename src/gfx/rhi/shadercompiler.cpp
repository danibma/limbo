#include "stdafx.h"
#include "shadercompiler.h"

#include "core/utils.h"

namespace limbo::RHI::SC
{
	bool Compile(Kernel& result, const char* programName, const char* entryPoint, KernelType kernel )
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
        std::string cpath = std::string("shaders/" + std::string(programName) + ".hlsl");
        std::wstring path;
        Utils::StringConvert(cpath, path);

        std::wstring profile = L"-T ";
        switch (kernel)
        {
        case KernelType::Compute:
            profile.append(L"cs");
            break;
        case KernelType::Vertex:
            profile.append(L"vs");
            break;
        case KernelType::Pixel:
            profile.append(L"ps");
            break;
        case KernelType::Lib:
            profile.append(L"lib");
        }
        profile.append(L"_6_6");


        std::vector<const wchar_t*> arguments;
        arguments.reserve(20);
        arguments.emplace_back(path.c_str());

		std::wstring wentrypoint;
        if (kernel != KernelType::Lib)
        {
	        Utils::StringConvert(entryPoint, wentrypoint);
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
        ComPtr<IDxcBlobEncoding> source = nullptr;
        dxcUtils->LoadFile(path.c_str(), nullptr, &source);
        if (!source)
        {
            LB_ERROR("Could not find the program shader file: %s", cpath.c_str());
            return false;
        }

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = source->GetBufferPointer();
        sourceBuffer.Size = source->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        ComPtr<IDxcResult> compileResult;
        DX_CHECK(dxcCompiler->Compile(&sourceBuffer, arguments.data(), (uint32)arguments.size(), includeHandler, IID_PPV_ARGS(&compileResult)));

        // Print errors if present.
        ComPtr<IDxcBlobUtf8> errors = nullptr;
        DX_CHECK(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
        // Note that d3dcompiler would return null if no errors or warnings are present.  
        // IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
        if (errors != nullptr && errors->GetStringLength() != 0)
        {
            LB_ERROR("%s", errors->GetStringPointer());
            return false;
        }

        // Get shader blob
        DX_CHECK(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&result.Bytecode), nullptr));

        return true;
	}
}
