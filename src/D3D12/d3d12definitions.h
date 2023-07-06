#pragma once

#include "core.h"
#include "definitions.h"

#include <comdef.h>

namespace limbo::rhi
{
	namespace internal
	{
		inline void dxHandleError(HRESULT hr)
		{
			_com_error err(hr);
			const char* errMsg = err.ErrorMessage();
			LB_ERROR("D3D12 Error: %s", errMsg);
		}
	}
}
#define DX_CHECK(expression) { HRESULT result = expression; if (result != S_OK) limbo::rhi::internal::dxHandleError(result); }
