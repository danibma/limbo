#include "stdafx.h"
#include "input.h"

#include "window.h"

namespace limbo::input
{

#define IMPLEMENT_INPUT_EVENT_PARAM(returnType, functionName, codeType) \
	returnType functionName(core::Window* window, codeType code) \
	{ \
		return window->functionName(code); \
	}

	IMPLEMENT_INPUT_EVENT_PARAM(bool, isKeyPressed, KeyCode);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, isKeyDown, KeyCode);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, isKeyUp, KeyCode);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, isMouseButtonPressed, MouseButton);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, isMouseButtonDown, MouseButton);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, isMouseButtonUp, MouseButton);

#define IMPLEMENT_INPUT_EVENT(returnType, functionName) \
	returnType functionName(core::Window* window) \
	{ \
		return window->functionName(); \
	}

	IMPLEMENT_INPUT_EVENT(float2, getMousePos);
	IMPLEMENT_INPUT_EVENT(float, getScrollX);
	IMPLEMENT_INPUT_EVENT(float, getScrollY);
}
