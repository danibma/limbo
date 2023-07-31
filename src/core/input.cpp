#include "stdafx.h"
#include "input.h"

#include "window.h"

namespace limbo::Input
{

#define IMPLEMENT_INPUT_EVENT_PARAM(returnType, functionName, codeType) \
	returnType functionName(Core::Window* window, codeType code) \
	{ \
		return window->functionName(code); \
	}

	IMPLEMENT_INPUT_EVENT_PARAM(bool, IsKeyPressed, KeyCode);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, IsKeyDown, KeyCode);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, IsKeyUp, KeyCode);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, IsMouseButtonPressed, MouseButton);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, IsMouseButtonDown, MouseButton);
	IMPLEMENT_INPUT_EVENT_PARAM(bool, IsMouseButtonUp, MouseButton);

#define IMPLEMENT_INPUT_EVENT(returnType, functionName) \
	returnType functionName(Core::Window* window) \
	{ \
		return window->functionName(); \
	}

	IMPLEMENT_INPUT_EVENT(float2, GetMousePos);
	IMPLEMENT_INPUT_EVENT(float , GetScrollX);
	IMPLEMENT_INPUT_EVENT(float , GetScrollY);
}
