#include "stdafx.h"
#include "window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace limbo::core
{
	Window* createWindow(const WindowInfo&& info)
	{
		Window* result = new Window(info);
		return result;
	}

	void destroyWindow(Window* window)
	{
		window->destroy();
		delete window;
	}

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		ensure(win);

		win->m_keysDown[key] = (action == GLFW_PRESS || action == GLFW_REPEAT);
	}

	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		ensure(win);

		win->m_buttonsDown[button] = action == GLFW_PRESS;
	}

	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		ensure(win);

		win->m_scroll = { xoffset, yoffset };
	}

	static void window_close_callback(GLFWwindow* window)
	{
		Window* win = (Window*)glfwGetWindowUserPointer(window);
		ensure(win);

		win->onWindowShouldClose.Broadcast();
	}

	Window::Window(const WindowInfo& info)
		: width(info.width), height(info.height)
	{
		if (!glfwInit())
		{
			LB_ERROR("Failed to initialize GLFW!");
			return;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_handle = glfwCreateWindow(width, height, "limbo", nullptr, nullptr);
		if (!m_handle)
		{
			LB_ERROR("Failed to create GLFW window!");
			return;
		}

		glfwSetWindowUserPointer(m_handle, this);

		glfwSetKeyCallback(m_handle, key_callback);
		glfwSetMouseButtonCallback(m_handle, mouse_button_callback);
		glfwSetScrollCallback(m_handle, scroll_callback);
		glfwSetWindowCloseCallback(m_handle, window_close_callback);
	}

	void Window::pollEvents()
	{
		memcpy(m_lastKeysDown, m_keysDown, sizeof(m_keysDown));
		memcpy(m_lastButtonsDown, m_buttonsDown, sizeof(m_buttonsDown));
		m_scroll = { 0, 0 };
		glfwPollEvents();
	}

	void Window::destroy()
	{
		glfwDestroyWindow(m_handle);
	}

	bool Window::shouldClose()
	{
		return glfwWindowShouldClose(m_handle);
	}

	GLFWwindow* Window::getGLFWHandle()
	{
		return m_handle;
	}

	HWND Window::getWin32Handle()
	{
		return glfwGetWin32Window(m_handle);
	}

	//
	// Input
	//
	bool Window::isKeyPressed(input::KeyCode key)
	{
		return m_keysDown[(int)key] && !m_lastKeysDown[(int)key];
	}

	bool Window::isKeyDown(input::KeyCode key)
	{
		ensure(key != input::KeyCode::Unknown);
		return m_keysDown[(int)key];
	}

	bool Window::isKeyUp(input::KeyCode key)
	{
		ensure(key != input::KeyCode::Unknown);
		return !m_keysDown[(int)key];
	}

	bool Window::isMouseButtonPressed(input::MouseButton button)
	{
		return m_buttonsDown[(int)button] && !m_lastButtonsDown[(int)button];
	}

	bool Window::isMouseButtonDown(input::MouseButton button)
	{
		return m_buttonsDown[(int)button];
	}

	bool Window::isMouseButtonUp(input::MouseButton button)
	{
		return !m_buttonsDown[(int)button];
	}

	float2 Window::getMousePos()
	{
		double x, y;
		glfwGetCursorPos(m_handle, &x, &y);
		return { x, y };
	}

	float Window::getScrollX()
	{
		return m_scroll.x;
	}

	float Window::getScrollY()
	{
		return m_scroll.y;
	}
}
