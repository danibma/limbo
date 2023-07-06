﻿#pragma once

#include "device.h"

#include <d3d12/d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>

using namespace Microsoft::WRL;

namespace limbo
{
	struct WindowInfo;
}

namespace limbo::rhi
{
	class D3D12Device final : public Device
	{
		ComPtr<IDXGIFactory2>			m_factory;
		ComPtr<IDXGIAdapter1>			m_adapter;
		ComPtr<ID3D12Device>			m_device;

	public:
		D3D12Device(const WindowInfo& info);
		virtual ~D3D12Device();

		void copyTextureToBackBuffer(Handle<Texture> texture) override;

		void bindDrawState(const DrawInfo& drawState) override;
		void draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) override;

		void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) override;

		void present() override;

	private:
		void pickGPU();
	};
}
