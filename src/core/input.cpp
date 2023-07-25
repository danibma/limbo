#include "input.h"

#include "window.h"

namespace limbo::input
{

#define IMPLEMENT_INPUT_EVENT(functionName, codeType) \
	bool functionName(core::Window* window, codeType code) \
	{ \
		return window->functionName(code); \
	}

	IMPLEMENT_INPUT_EVENT(isKeyPressed, KeyCode);
	IMPLEMENT_INPUT_EVENT(isKeyDown, KeyCode);
	IMPLEMENT_INPUT_EVENT(isKeyUp, KeyCode);
	IMPLEMENT_INPUT_EVENT(isMouseButtonPressed, MouseButton);
	IMPLEMENT_INPUT_EVENT(isMouseButtonDown, MouseButton);
	IMPLEMENT_INPUT_EVENT(isMouseButtonUp, MouseButton);

	float2 getMousePos(core::Window* window)
	{
		return window->getMousePos();
	}
}
