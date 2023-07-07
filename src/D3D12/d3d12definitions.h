#pragma once

#include "core.h"
#include "definitions.h"

#include <d3d12/d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>

using namespace Microsoft::WRL;

namespace limbo::rhi
{
	namespace internal
	{
		void dxHandleError(HRESULT hr);
	}
}
#define DX_CHECK(expression) { HRESULT result = expression; if (result != S_OK) limbo::rhi::internal::dxHandleError(result); }
