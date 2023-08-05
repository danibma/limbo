#include "stdafx.h"
#include "window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace limbo::Core
{
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		ensure(win);

		win->m_KeysDown[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
	}

	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		ensure(win);

		win->m_ButtonsDown[button] = action == GLFW_PRESS;
	}

	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		ensure(win);

		win->m_Scroll = { xoffset, yoffset };
	}

	static void SizeCallback(GLFWwindow* window, int width, int height)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		ensure(win);

		win->Width  = width;
		win->Height = height;
		win->OnWindowResize.Broadcast(width, height);
	}

	Window::Window(const WindowInfo& info)
		: Width(info.Width), Height(info.Height)
	{
		if (!glfwInit())
		{
			LB_ERROR("Failed to initialize GLFW!");
			return;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		if (info.Maximized)
			glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

		m_Handle = glfwCreateWindow(Width, Height, "limbo", nullptr, nullptr);
		if (!m_Handle)
		{
			LB_ERROR("Failed to create GLFW window!");
			return;
		}

		if (info.Maximized)
		{
			int w, h;
			glfwGetWindowSize(m_Handle, &w, &h);
			Width = w;
			Height = h;
		}
		
		glfwSetWindowUserPointer(m_Handle, this);

		glfwSetKeyCallback(m_Handle, KeyCallback);
		glfwSetMouseButtonCallback(m_Handle, MouseButtonCallback);
		glfwSetScrollCallback(m_Handle, ScrollCallback);
		glfwSetFramebufferSizeCallback(m_Handle, SizeCallback);
	}

	void Window::PollEvents()
	{
		memcpy(m_LastKeysDown, m_KeysDown, sizeof(m_KeysDown));
		memcpy(m_LastButtonsDown, m_ButtonsDown, sizeof(m_ButtonsDown));
		m_Scroll = { 0, 0 };
		glfwPollEvents();
	}

	void Window::Destroy()
	{
		glfwDestroyWindow(m_Handle);
	}

	bool Window::ShouldClose()
	{
		bool shouldClose = glfwWindowShouldClose(m_Handle);
		if (shouldClose)
			OnWindowShouldClose.Broadcast();
		return shouldClose;
	}

	GLFWwindow* Window::GetGlfwHandle()
	{
		return m_Handle;
	}

	HWND Window::GetWin32Handle()
	{
		return glfwGetWin32Window(m_Handle);
	}

	//
	// Input
	//
	bool Window::IsKeyPressed(Input::KeyCode key)
	{
		return m_KeysDown[(int)key] && !m_LastKeysDown[(int)key];
	}

	bool Window::IsKeyDown(Input::KeyCode key)
	{
		ensure(key != Input::KeyCode::Unknown);
		return m_KeysDown[(int)key];
	}

	bool Window::IsKeyUp(Input::KeyCode key)
	{
		ensure(key != Input::KeyCode::Unknown);
		return !m_KeysDown[(int)key];
	}

	bool Window::IsMouseButtonPressed(Input::MouseButton button)
	{
		return m_ButtonsDown[(int)button] && !m_LastButtonsDown[(int)button];
	}

	bool Window::IsMouseButtonDown(Input::MouseButton button)
	{
		return m_ButtonsDown[(int)button];
	}

	bool Window::IsMouseButtonUp(Input::MouseButton button)
	{
		return !m_ButtonsDown[(int)button];
	}

	float2 Window::GetMousePos()
	{
		double x, y;
		glfwGetCursorPos(m_Handle, &x, &y);
		return { x, y };
	}

	float Window::GetScrollX()
	{
		return m_Scroll.x;
	}

	float Window::GetScrollY()
	{
		return m_Scroll.y;
	}
}
