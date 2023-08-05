#pragma once

#include "core.h"
#include "input.h"

#include <CppDelegates/Delegates.h>

struct GLFWwindow;

namespace limbo::Core
{
	struct WindowInfo
	{
		const char*		Title;
		uint32			Width;
		uint32			Height;
		bool			Maximized = true;
	};

	class Window
	{
	private:
		GLFWwindow*			m_Handle;

		// input
		bool				m_KeysDown[(int32)Input::KeyCode::Menu] = {};
		bool				m_LastKeysDown[(int32)Input::KeyCode::Menu] = {};
		bool				m_ButtonsDown[(uint32)Input::MouseButton::B8] = {};
		bool				m_LastButtonsDown[(uint32)Input::MouseButton::B8] = {};

		float2				m_Scroll = { 0, 0 };

	public:
		uint32				Width;
		uint32				Height;

	public:
		Window() = default;
		Window(const WindowInfo& info);

		void PollEvents();
		void Destroy();

		bool ShouldClose();

		GLFWwindow* GetGlfwHandle();
		HWND GetWin32Handle();

		DECLARE_MULTICAST_DELEGATE(TOnWindowShouldClose);
		TOnWindowShouldClose OnWindowShouldClose;

		DECLARE_MULTICAST_DELEGATE(TOnWindowResize, uint32, uint32);
		TOnWindowResize OnWindowResize;

	private:
		bool IsKeyPressed(Input::KeyCode key);
		bool IsKeyDown(Input::KeyCode key);
		bool IsKeyUp(Input::KeyCode key);

		bool IsMouseButtonPressed(Input::MouseButton button);
		bool IsMouseButtonDown(Input::MouseButton button);
		bool IsMouseButtonUp(Input::MouseButton button);

		float2 GetMousePos();
		float GetScrollX();
		float GetScrollY();

		// GLFW
		friend void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
		friend void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
		friend void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
		friend void SizeCallback(GLFWwindow* window, int width, int height);

		// Input
		friend bool Input::IsKeyPressed(Core::Window* window, Input::KeyCode key);
		friend bool Input::IsKeyDown(Core::Window* window, Input::KeyCode key);
		friend bool Input::IsKeyUp(Core::Window* window, Input::KeyCode key);
		friend bool Input::IsMouseButtonPressed(Core::Window* window, Input::MouseButton button);
		friend bool Input::IsMouseButtonDown(Core::Window* window, Input::MouseButton button);
		friend bool Input::IsMouseButtonUp(Core::Window* window, Input::MouseButton button);
		friend float2 Input::GetMousePos(Core::Window* window);
		friend float Input::GetScrollX(Core::Window* window);
		friend float Input::GetScrollY(Core::Window* window);
	};

	inline Window* NewWindow(const WindowInfo&& info)
	{
		Window* result = new Window(info);
		return result;
	}

	inline void DestroyWindow(Window* window)
	{
		window->Destroy();
		delete window;
	}
}
