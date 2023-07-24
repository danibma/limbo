﻿#include "shadercompiler.h"

#include "core/utils.h"

namespace limbo::gfx::SC
{
	bool compile(Kernel& result, const char* programName, const char* entryPoint, KernelType kernel )
	{
        static IDxcUtils* dxcUtils;
        static IDxcCompiler3* dxcCompiler;
        static IDxcIncludeHandler* includeHandler;
        if (!dxcCompiler)
        {
            DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
            DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
            dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
        }

        // setup arguments
        std::string cpath = std::string("shaders/" + std::string(programName) + ".hlsl");
        std::wstring path;
        utils::StringConvert(cpath, path);

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
        }
        profile.append(L"_6_0");

        std::wstring wentrypoint;
        utils::StringConvert(entryPoint, wentrypoint);
        wentrypoint.insert(0, L"-E ");

        std::vector<const wchar_t*> arguments;
        arguments.reserve(20);
        arguments.emplace_back(path.c_str());
        arguments.emplace_back(wentrypoint.c_str());
        arguments.emplace_back(profile.c_str());
        
        // Debug: Disable optimizations and embed HLSL source in the shaders
#if LIMBO_DEBUG
        arguments.emplace_back(DXC_ARG_SKIP_OPTIMIZATIONS); // Disable optimizations
        arguments.emplace_back(DXC_ARG_DEBUG);              // Enable debug information
        arguments.emplace_back(L"-Qembed_debug");           // Embed PDB in shader container (must be used with /Zi)
#endif

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
            LB_ERROR("Failed to compile %s shader: %s", cpath.c_str(), errors->GetStringPointer());
            return false;
        }

        // Get shader reflection data.
        ComPtr<IDxcBlob> reflectionBlob;
        DX_CHECK(compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionBlob), nullptr));

        const DxcBuffer reflectionBuffer = {
            .Ptr = reflectionBlob->GetBufferPointer(),
            .Size = reflectionBlob->GetBufferSize(),
            .Encoding = 0,
        };

        DX_CHECK(dxcUtils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&result.reflection)));

        // Get shader blob
        DX_CHECK(compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&result.bytecode), nullptr));

        return true;
	}
}