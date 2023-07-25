#pragma once

#include "core.h"
#include "input.h"

struct GLFWwindow;

namespace limbo::core
{
	struct WindowInfo
	{
		const char*		title;
		uint32			width;
		uint32			height;
	};

	class Window
	{
	private:
		GLFWwindow*			m_handle;

		// input
		bool				m_keysDown[(int32)input::KeyCode::Menu] = {};
		bool				m_lastKeysDown[(int32)input::KeyCode::Menu] = {};
		bool				m_buttonsDown[(uint32)input::MouseButton::B8] = {};
		bool				m_lastButtonsDown[(uint32)input::MouseButton::B8] = {};

		float2				m_scroll = { 0, 0 };

	public:
		uint32				width;
		uint32				height;

	public:
		Window() = default;
		Window(const WindowInfo& info);

		void pollEvents();
		void destroy();

		bool shouldClose();

		HWND getWin32Handle();

	private:
		bool isKeyPressed(input::KeyCode key);
		bool isKeyDown(input::KeyCode key);
		bool isKeyUp(input::KeyCode key);

		bool isMouseButtonPressed(input::MouseButton button);
		bool isMouseButtonDown(input::MouseButton button);
		bool isMouseButtonUp(input::MouseButton button);

		float2 getMousePos();
		float getScrollX();
		float getScrollY();

		// GLFW
		friend void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
		friend void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
		friend void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

		// Input
		friend bool input::isKeyPressed(core::Window* window, input::KeyCode key);
		friend bool input::isKeyDown(core::Window* window, input::KeyCode key);
		friend bool input::isKeyUp(core::Window* window, input::KeyCode key);
		friend bool input::isMouseButtonPressed(core::Window* window, input::MouseButton button);
		friend bool input::isMouseButtonDown(core::Window* window, input::MouseButton button);
		friend bool input::isMouseButtonUp(core::Window* window, input::MouseButton button);
		friend float2 input::getMousePos(core::Window* window);
		friend float input::getScrollX(core::Window* window);
		friend float input::getScrollY(core::Window* window);
	};

	Window* createWindow(const WindowInfo&& info);
	void destroyWindow(Window* window);
}
