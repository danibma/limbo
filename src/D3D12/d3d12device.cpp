#include "d3d12device.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

namespace limbo
{
	rhi::D3D12Device::D3D12Device(const WindowInfo& info)
	{
		D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&m_device));
	}

	rhi::D3D12Device::~D3D12Device()
	{
	}

	void rhi::D3D12Device::copyTextureToBackBuffer(Handle<Texture> texture)
	{
	}

	void rhi::D3D12Device::bindDrawState(const DrawInfo& drawState)
	{
	}

	void rhi::D3D12Device::draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)
	{
	}

	void rhi::D3D12Device::dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
	}

	void rhi::D3D12Device::present()
	{
	}
}
