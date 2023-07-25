#pragma once

#include "core/core.h"
#include "core/math.h"

namespace limbo::core
{
	class Window;
}

namespace limbo::input
{
	// wrapper of the GLFW_KEY codes
	enum class KeyCode : int32
	{
		Unknown = -1,
		Space = 32,
		Apostrophe = 39,
		Comma = 44,
		Minus,
		Period,
		Slash,
		K0,
		K1,
		K2,
		K3,
		K4,
		K5,
		K6,
		K7,
		K8,
		K9,
		Semicolon = 59,
		Equal = 61,
		A = 65,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		LeftBracket,
		Backslash,
		RightBracket,
		GraveAccent = 96,
		World1 = 161,
		World2 = 162,
		Escape = 256,
		Enter,
		Tab,
		Backspace,
		Insert,
		Delete,
		Right,
		Left,
		Down,
		Up,
		PageUp,
		PageDown,
		Home,
		End,
		CapsLock = 280,
		ScrollLock,
		NumLock,
		PrintScreen,
		Pause,
		F1 = 290,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		F21,
		F22,
		F23,
		F24,
		F25,
		KP_0 = 320,
		KP_1,
		KP_2,
		KP_3,
		KP_4,
		KP_5,
		KP_6,
		KP_7,
		KP_8,
		KP_9,
		KP_Decimal,
		KP_Divide,
		KP_Multiply,
		KP_Subtract,
		KP_Add,
		KP_Enter,
		KP_Equal,
		LeftShift = 340,
		LeftControl,
		LeftAlt,
		LeftSuper,
		RightShift,
		RightControl,
		RightAlt,
		RightSuper,
		Menu
	};

	// wrapper of the GLFW_MOUSE_BUTTON codes
	enum class MouseButton : uint32
	{
		B1 = 0,
		B2,
		B3,
		B4,
		B5,
		B6,
		B7,
		B8,
		Left = B1,
		Right = B2,
		Middle = B3,
	};

	enum class CursorMode : uint32
	{
		Visible = 0x00034001, // GLFW_NORMAL
		Hidden = 0x00034002, // GLFW_HIDDEN
		Disabled = 0x00034003  // GLFW_DISABLED
	};

#define DECLARE_INPUT_EVENT(functionName, codeType)\
	bool functionName(core::Window* window, codeType code)

	DECLARE_INPUT_EVENT(isKeyPressed, KeyCode);
	DECLARE_INPUT_EVENT(isKeyDown, KeyCode);
	DECLARE_INPUT_EVENT(isKeyUp, KeyCode);
	DECLARE_INPUT_EVENT(isMouseButtonPressed, MouseButton);
	DECLARE_INPUT_EVENT(isMouseButtonDown, MouseButton);
	DECLARE_INPUT_EVENT(isMouseButtonUp, MouseButton);

	float2 getMousePos(core::Window* window);
}
