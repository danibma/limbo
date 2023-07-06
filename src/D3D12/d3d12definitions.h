#pragma once

#include "core.h"
#include "definitions.h"

namespace limbo::rhi
{
	namespace internal
	{
		void dxHandleError(HRESULT hr);
	}
}
#define DX_CHECK(expression) { HRESULT result = expression; if (result != S_OK) limbo::rhi::internal::dxHandleError(result); }
