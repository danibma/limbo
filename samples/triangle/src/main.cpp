#include <limbo.h>
#include <math.h>

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
	if (!glfwInit())
	{
		LB_ERROR("Failed to initialize GLFW!");
		return 0;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(1280, 720, "Triangle", nullptr, nullptr);
	if (!window)
	{
		LB_ERROR("Failed to create GLFW window!");
		return 0;
	}

	
	limbo::init({ 
#if LIMBO_WINDOWS
		.hwnd = glfwGetWin32Window(window),
#elif LIMBO_LINUX
		.hwnd = glfwGetX11Window(window),
#endif
		.width = 1280,
		.height = 720 });

#define COMPUTE 1
#if COMPUTE
	limbo::Handle<limbo::Shader> triangleCSShader = limbo::createShader({});
	limbo::Handle<limbo::Texture> outputTexture = limbo::createTexture({ .width = 1280, .height = 720 });
#else
	float vertices[] = { 0.5f, -0.5f, 0.0f,
						  0.0f,  0.7f, 0.0f,
						 -0.5f, -0.5f, 0.0f };
	limbo::Handle<limbo::Buffer> vertexBuffer = limbo::createBuffer({ 
		.debugName = "triangle vb", 
		.byteSize = 150, 
		.usage = limbo::BufferUsage::Vertex, 
		.initialData = vertices });

	limbo::Handle<limbo::Shader> triangleShader = limbo::createShader({});
#endif

	for (float time = 0.0f; !glfwWindowShouldClose(window); time += 0.1f)
	{
		glfwPollEvents();
		Noop(time);

#if COMPUTE
		limbo::setParameter(triangleCSShader, 0, outputTexture);
		limbo::bindShader(triangleCSShader);
		limbo::dispatch(1280 / 8, 720 / 8, 1);

		limbo::copyTextureToBackBuffer(outputTexture);

		limbo::present();
#else
		float color[] = { 0.5f * cosf(time) + 0.5f,
						  0.5f * sinf(time) + 0.5f,
						  1.0f };

		limbo::setParameter(triangleShader, 0, color);
		limbo::bindShader(triangleShader);
		limbo::bindVertexBuffer(vertexBuffer);

		limbo::draw(3);
		limbo::present();
#endif
	}

#if COMPUTE
	limbo::destroyTexture(outputTexture);
	limbo::destroyShader(triangleCSShader);
#else
	limbo::destroyBuffer(vertexBuffer);
	limbo::destroyShader(triangleShader);
#endif

	limbo::shutdown();
	glfwTerminate();

	return 0;
}