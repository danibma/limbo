#include <limbo.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if LIMBO_WINDOWS
#	define GLFW_EXPOSE_NATIVE_WIN32
#else
#	define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>

int main(int argc, char* argv[])
{
	GLFWwindow* window;

	if (!glfwInit())
	{
		LB_ERROR("Failed to initialize GLFW!");
		return 0;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(1280, 720, "Triangle", nullptr, nullptr);
	if (!window)
	{
		LB_ERROR("Failed to create GLFW window!");
		return 0;
	}

	
#if LIMBO_WINDOWS
	limbo::init(glfwGetWin32Window(window));
#elif LIMBO_LINUX
	limbo::init(glfwGetX11Window(window));
#endif

	float vertices[] = { 0.5f, -0.5f, 0.0f,
						  0.0f,  0.7f, 0.0f,
						 -0.5f, -0.5f, 0.0f };
	limbo::Handle<limbo::Buffer> vertexBuffer = limbo::createBuffer({ .debugName = "triangle vb", .byteSize = 150, .initialData = vertices });

	/*
	auto vertex_buffer = gfxCreateBuffer(gfx, sizeof(vertices), vertices);

	auto program = gfxCreateProgram(gfx, "triangle");
	auto kernel = gfxCreateGraphicsKernel(gfx, program);*/

	for (float time = 0.0f; !glfwWindowShouldClose(window); time += 0.1f)
	{
		glfwPollEvents();
		Noop(time);

		/*float color[] = { 0.5f * cosf(time) + 0.5f,
						  0.5f * sinf(time) + 0.5f,
						  1.0f };
		gfxProgramSetParameter(gfx, program, "Color", color);

		gfxCommandBindKernel(gfx, kernel);
		gfxCommandBindVertexBuffer(gfx, vertex_buffer);

		gfxCommandDraw(gfx, 3);

		gfxFrame(gfx);*/
	}

	limbo::shutdown();
	glfwTerminate();

	return 0;
}