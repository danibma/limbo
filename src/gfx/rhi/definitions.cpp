#include "definitions.h"

#include <comdef.h>

namespace limbo::gfx
{
	namespace internal
	{
		void dxHandleError(HRESULT hr)
		{
			_com_error err(hr);
			const char* errMsg = err.ErrorMessage();
			LB_ERROR("D3D12 Error: %s", errMsg);
		}
	}
}
