#include "definitions.h"

#include <comdef.h>

namespace limbo::gfx
{
	namespace internal
	{
		void dxHandleError(HRESULT hr, const char* file, int line)
		{
			_com_error err(hr);
			const char* errMsg = err.ErrorMessage();
			LB_ERROR("D3D12 Error: %s in %s:%d", errMsg, file, line);
		}

		void dxMessageCallback(D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID, LPCSTR pDescription, void* pContext)
		{
			switch (Severity)
			{
			case D3D12_MESSAGE_SEVERITY_CORRUPTION:
			case D3D12_MESSAGE_SEVERITY_ERROR:
				LB_ERROR("%s", pDescription);
				break;
			case D3D12_MESSAGE_SEVERITY_WARNING:
				LB_WARN("%s", pDescription);
				break;
			case D3D12_MESSAGE_SEVERITY_INFO:
			case D3D12_MESSAGE_SEVERITY_MESSAGE:
				LB_LOG("%s", pDescription);
				break;
			}
		}
	}
}
