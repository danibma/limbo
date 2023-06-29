#include "vulkandevice.h"
#include "vulkandefinitions.h"

#if LIMBO_WINDOWS
#include <Windows.h>
#endif

namespace limbo
{
	void loadVulkan()
	{
#if LIMBO_WINDOWS
		static HMODULE module;
		if (!module) module = ::LoadLibraryA("vulkan-1.dll");

#define LOAD_VK_FUNCTION(Type, Func) vk::Func = (Type)::GetProcAddress(module, #Func);
#elif LIMBO_LINUX
		void* module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
		if (!module)
			module = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
		ensure(module);
#define LOAD_VK_FUNCTION(Type, Func) vk::Func = (Type)dlsym(module, #Func);
#endif

		ENUM_VK_ENTRYPOINTS_BASE(LOAD_VK_FUNCTION);
	}

	VulkanDevice::VulkanDevice()
	{
		loadVulkan();

		VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "limbo",
			.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
			.pEngineName = "limbo",
			.engineVersion = VK_MAKE_VERSION(0, 0, 1),
			.apiVersion = VK_VERSION_1_3,
		};

		VkInstanceCreateInfo instanceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = 0,
			.enabledExtensionCount = 0,
		};

		VkInstance instance;
		VK_CHECK(vk::vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
	}

	VulkanDevice::~VulkanDevice()
	{

	}

	void VulkanDevice::setParameter(Handle<Shader> shader, uint8 slot, const void* data)
	{
	}

	void VulkanDevice::setParameter(Handle<Shader> shader, uint8 slot, Handle<Buffer> buffer)
	{
	}

	void VulkanDevice::setParameter(Handle<Shader> shader, uint8 slot, Handle<Texture> texture)
	{
	}

	void VulkanDevice::bindShader(Handle<Shader> shader)
	{
	}

	void VulkanDevice::bindVertexBuffer(Handle<Buffer> vertexBuffer)
	{
	}

	void VulkanDevice::bindIndexBuffer(Handle<Buffer> indexBuffer)
	{
	}

	void VulkanDevice::draw(uint32 vertexCount, uint32 instanceCount /*= 1*/, uint32 firstVertex /*= 1*/, uint32 firstInstance /*= 1*/)
	{
	}

	void VulkanDevice::present()
	{
	}

}
