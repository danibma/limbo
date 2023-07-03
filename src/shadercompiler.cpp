#include "shadercompiler.h"

#include <Windows.h>
#include <dxcapi.h>
#include <string>
#include <vector>

#include "utils.h"

namespace limbo::rhi::ShaderCompiler
{
	bool compile(ShaderResult& result, const char* programName, const char* entryPoint, KernelType kernel, bool bIsSpirV)
	{
        static IDxcUtils* m_dxcUtils;
        static IDxcCompiler3* m_dxcCompiler;
        static IDxcIncludeHandler* m_includeHandler;
        if (!m_dxcCompiler)
        {
            DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxcUtils));
            DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxcCompiler));
            m_dxcUtils->CreateDefaultIncludeHandler(&m_includeHandler);
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

        std::wstring uType = std::to_wstring(ShiftUType);
        std::wstring bType = std::to_wstring(ShiftBType);
        std::wstring tType = std::to_wstring(ShiftTType);
        std::wstring sType = std::to_wstring(ShiftSType);

        // SPIR-V
        if (bIsSpirV)
        {
            arguments.emplace_back(L"-spirv");                     // Generate SPIR-V code
            arguments.emplace_back(L"-fspv-target-env=vulkan1.3"); // Specify the target environment: vulkan1.0 (default), vulkan1.1, vulkan1.1spirv1.4, vulkan1.2, vulkan1.3, or universal1.5

            arguments.emplace_back(L"-fvk-use-dx-layout");     // Use DirectX memory layout for Vulkan resources
            arguments.emplace_back(L"-fvk-use-dx-position-w"); // Reciprocate SV_Position.w after reading from stage input in PS to accommodate the difference between Vulkan and DirectX

            // Shift registers to avoid conflicts
            arguments.emplace_back(L"-fvk-u-shift"); arguments.emplace_back(uType.c_str()); arguments.emplace_back(L"all"); // Specify Vulkan binding number shift for u-type (read/write buffer) register
            arguments.emplace_back(L"-fvk-b-shift"); arguments.emplace_back(bType.c_str()); arguments.emplace_back(L"all"); // Specify Vulkan binding number shift for b-type (buffer) register
            arguments.emplace_back(L"-fvk-t-shift"); arguments.emplace_back(tType.c_str()); arguments.emplace_back(L"all"); // Specify Vulkan binding number shift for t-type (texture) register
            arguments.emplace_back(L"-fvk-s-shift"); arguments.emplace_back(sType.c_str()); arguments.emplace_back(L"all"); // Specify Vulkan binding number shift for s-type (sampler) register

            if (kernel == KernelType::Vertex)
                arguments.emplace_back(L"-fvk-invert-y"); // Negate SV_Position.y before writing to stage output in VS / DS / GS to accommodate Vulkan's coordinate system
        }
        
        // Debug: Disable optimizations and embed HLSL source in the shaders
#if LIMBO_DEBUG
        arguments.emplace_back(L"-Od");           // Disable optimizations
        arguments.emplace_back(L"-Zi");           // Enable debug information
        arguments.emplace_back(L"-Qembed_debug"); // Embed PDB in shader container (must be used with /Zi)
#endif

        // Compile shader
        IDxcBlobEncoding* source = nullptr;
        m_dxcUtils->LoadFile(path.c_str(), nullptr, &source);
        if (!source)
        {
            LB_ERROR("Could not find the program shader file: %s", cpath.c_str());
            return false;
        }

        DxcBuffer sourceBuffer;
        sourceBuffer.Ptr = source->GetBufferPointer();
        sourceBuffer.Size = source->GetBufferSize();
        sourceBuffer.Encoding = DXC_CP_ACP;

        IDxcResult* compileResult;
        m_dxcCompiler->Compile(&sourceBuffer, arguments.data(), (uint32)arguments.size(), m_includeHandler, IID_PPV_ARGS(&compileResult));

        // Print errors if present.
        IDxcBlobUtf8* errors = nullptr;
        compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        // Note that d3dcompiler would return null if no errors or warnings are present.  
        // IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
        if (errors != nullptr && errors->GetStringLength() != 0)
        {
            LB_ERROR("Failed to compile %s shader: %s", cpath.c_str(), errors->GetStringPointer());
            return false;
        }

        // Get shader blob
        IDxcBlob* shaderBlob;
        compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);

        result.code = shaderBlob->GetBufferPointer();
        result.size = shaderBlob->GetBufferSize();

        return true;
	}
}
