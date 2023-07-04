#include "vulkandevice.h"
#include "vulkandefinitions.h"
#include "vulkanresourcemanager.h"
#include "vulkanshader.h"
#include "vulkanswapchain.h"

#if LIMBO_WINDOWS
#include <Windows.h>
#endif

namespace limbo::rhi
{
	void loadVulkanBaseLibrary()
	{
#if LIMBO_WINDOWS
		static HMODULE module;
		if (!module) module = ::LoadLibraryA("vulkan-1.dll");

#define LOAD_VK_BASE_FUNCTION(Type, Func) vk::Func = (Type)::GetProcAddress(module, #Func);
#elif LIMBO_LINUX
		void* module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
		if (!module)
			module = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
		ensure(module);
#define LOAD_VK_BASE_FUNCTION(Type, Func) vk::Func = (Type)dlsym(module, #Func);
#endif

		ENUM_VK_ENTRYPOINTS_BASE(LOAD_VK_BASE_FUNCTION);

	}

	void loadVulkanInstanceLibrary(VkInstance instance)
	{
#define LOAD_VK_INSTANCE_FUNCTION(Type, Func) vk::Func = (Type)vk::vkGetInstanceProcAddr(instance, #Func);
		ENUM_VK_ENTRYPOINTS_INSTANCE(LOAD_VK_INSTANCE_FUNCTION);
		ENUM_VK_ENTRYPOINTS_PLATFORM_INSTANCE(LOAD_VK_INSTANCE_FUNCTION);
	}

	VulkanDevice::VulkanDevice(const limbo::WindowInfo& info)
	{
		loadVulkanBaseLibrary();

		// Extensions
#if LIMBO_DEBUG
		m_instanceLayers.emplace_back("VK_LAYER_KHRONOS_validation");
		m_validationFeatures.emplace_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
		m_validationFeatures.emplace_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
		m_instanceExtensions.emplace_back("VK_EXT_debug_utils");
#endif
		m_instanceExtensions.emplace_back("VK_KHR_surface");
		m_instanceExtensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
		m_deviceExtensions.emplace_back("VK_KHR_swapchain");

		VkApplicationInfo appInfo = {
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "limbo",
			.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
			.pEngineName = "limbo",
			.engineVersion = VK_MAKE_VERSION(0, 0, 1),
			.apiVersion = VK_HEADER_VERSION_COMPLETE,
		};

		VkValidationFeaturesEXT validationFeatures = {
			.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
			.enabledValidationFeatureCount = (uint32)m_validationFeatures.size(),
			.pEnabledValidationFeatures = m_validationFeatures.data()
		};

		VkInstanceCreateInfo instanceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext = &validationFeatures,
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = (uint32)m_instanceLayers.size(),
			.ppEnabledLayerNames = m_instanceLayers.data(),
			.enabledExtensionCount = (uint32)m_instanceExtensions.size(),
			.ppEnabledExtensionNames = m_instanceExtensions.data(),
		};

		VK_CHECK(vk::vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance));

		loadVulkanInstanceLibrary(m_instance);

		findPhysicalDevice();
		ensure(m_gpu);

		vk::vkGetPhysicalDeviceMemoryProperties(m_gpu, &m_memoryProperties);

#if LIMBO_DEBUG
		VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vkDebugCallback,
		};
		vk::vkCreateDebugUtilsMessengerEXT(m_instance, &messengerCreateInfo, nullptr, &m_messenger);
#endif

		createLogicalDevice();

		m_swapchain = new VulkanSwapchain(this, info);

		// Create semaphores
		VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vk::vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_frame.m_acquireSemaphore));
		VK_CHECK(vk::vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_frame.m_renderSemaphore));

		// Create the command pool
		VkCommandPoolCreateInfo cmdPoolCreateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		cmdPoolCreateInfo.queueFamilyIndex = m_graphicsQueueFamily;
		VK_CHECK(vk::vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_frame.m_commandPool));

		// Allocate command buffers
		VkCommandBufferAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandPool = m_frame.m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;
		VK_CHECK(vk::vkAllocateCommandBuffers(m_device, &allocInfo, &m_frame.m_commandBuffer));

		// Create descriptor pool
		VkDescriptorPoolSize descriptorPoolSizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER,				 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,			 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,			 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,	 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,	 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,		 1000 }
		};

		VkDescriptorPoolCreateInfo descPoolInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = 1000,
			.poolSizeCount = sizeof(descriptorPoolSizes) / sizeof(descriptorPoolSizes[0]),
			.pPoolSizes = descriptorPoolSizes
		};
		VK_CHECK(vk::vkCreateDescriptorPool(m_device, &descPoolInfo, nullptr, &m_descriptorPool));

		vk::vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);

		// Reset the command pool
		resetFrame(m_frame);
	}

	void VulkanDevice::createLogicalDevice()
	{
		// Get queues
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfo;
		uint32_t queueFamilies[3] = { m_graphicsQueueFamily, m_computeQueueFamily, m_transferQueueFamily };
		float priority = 1.0f;
		for (uint32_t i = 0; i < 3; ++i)
		{
			if (i > 0 && queueFamilies[i] == queueFamilies[i - 1]) continue;

			VkDeviceQueueCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			createInfo.queueFamilyIndex = queueFamilies[i];
			createInfo.queueCount = 1;
			createInfo.pQueuePriorities = &priority;
			queueCreateInfo.push_back(createInfo);
		}

		// Enable device features
		VkPhysicalDeviceVulkan13Features deviceFeatures13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		VkPhysicalDeviceVulkan12Features deviceFeatures12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		deviceFeatures12.pNext = &deviceFeatures13;
		VkPhysicalDeviceVulkan11Features deviceFeatures11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
		deviceFeatures11.pNext = &deviceFeatures12;
		VkPhysicalDeviceFeatures2 deviceFeatures2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		deviceFeatures2.pNext = &deviceFeatures11;

		// Check feature support
		{
			// Check feature support
			VkPhysicalDeviceVulkan13Features featuresSupported13 = {};
			featuresSupported13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
			VkPhysicalDeviceVulkan12Features featuresSupported12 = {};
			featuresSupported12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			featuresSupported12.pNext = &featuresSupported13;
			VkPhysicalDeviceFeatures2 featuresSupported = {};
			featuresSupported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			featuresSupported.pNext = &featuresSupported12;
			vk::vkGetPhysicalDeviceFeatures2(m_gpu, &featuresSupported);

			// Line and point rendering
			ensure(featuresSupported.features.fillModeNonSolid == VK_TRUE);
			deviceFeatures2.features.fillModeNonSolid = VK_TRUE;

			// Rendering without render passes and frame buffer objects
			ensure(featuresSupported13.dynamicRendering == VK_TRUE);
			deviceFeatures13.dynamicRendering = VK_TRUE;

			// Synchronization 2
			ensure(featuresSupported13.synchronization2 == VK_TRUE);
			deviceFeatures13.synchronization2 = VK_TRUE;
		}

		VkDeviceCreateInfo deviceCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext = &deviceFeatures2,
			.queueCreateInfoCount = (uint32)queueCreateInfo.size(),
			.pQueueCreateInfos = queueCreateInfo.data(),
			.enabledExtensionCount = (uint32)m_deviceExtensions.size(),
			.ppEnabledExtensionNames = m_deviceExtensions.data(),
		};
		VK_CHECK(vk::vkCreateDevice(m_gpu, &deviceCreateInfo, nullptr, &m_device));
	}

	void VulkanDevice::resetFrame(const VulkanPerFrame& frame)
	{
		VK_CHECK(vk::vkResetCommandPool(m_device, frame.m_commandPool, 0));
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		VK_CHECK(vk::vkBeginCommandBuffer(frame.m_commandBuffer, &beginInfo));

		m_swapchain->prepareNextImage(frame);

		VkRenderingAttachmentInfo colorAttachInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = m_swapchain->getCurrentImageView(),
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR ,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = {
				.color = { 0.0f, 0.5f, 1.0f, 1.0f }
			}
		};

		VkRenderingInfo renderingInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = {
				.extent = {
					.width = m_swapchain->getImagesWidth(),
					.height = m_swapchain->getImagesHeight()
				}
			},
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachInfo,
			.pDepthAttachment = nullptr,
			.pStencilAttachment = nullptr 
		};
		//vk::vkCmdBeginRendering(frame.m_commandBuffer, &renderingInfo);
	}

	VulkanDevice::~VulkanDevice()
	{
		m_frame.destroy(m_device);

		delete m_swapchain;
		vk::vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);
		vk::vkDestroyDevice(m_device, nullptr);
		vk::vkDestroyInstance(m_instance, nullptr);
	}

	void VulkanDevice::copyTextureToBackBuffer(Handle<Texture> texture)
	{
	}

	void VulkanDevice::bindDrawState(const DrawInfo& drawState)
	{
		VulkanResourceManager* rm = ResourceManager::getAs<VulkanResourceManager>();

		VkCommandBuffer cmd = m_frame.m_commandBuffer;
		VulkanShader* shader = rm->getShader(drawState.shader);

		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
		if (shader->type == ShaderType::Compute)
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
		else if (shader->type == ShaderType::Graphics)
			bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		vk::vkCmdBindPipeline(cmd, bindPoint, shader->pipeline);

		const uint32 bindsCount = (uint32)drawState.bindGroups.size();
		std::vector<VkDescriptorSet> descriptorSets(bindsCount);
		for (uint32 i = 0; i < bindsCount; ++i)
		{
			VulkanBindGroup* bind = rm->getBindGroup(drawState.bindGroups[i]);
			if (!bind) continue;

			bind->update(m_device);

			descriptorSets[i] = bind->set;
		}

		vk::vkCmdBindDescriptorSets(cmd, bindPoint, shader->layout, 0, (uint32)descriptorSets.size(), descriptorSets.data(), 0, nullptr);
	}

	void VulkanDevice::dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		VkCommandBuffer cmd = m_frame.m_commandBuffer;
		vk::vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanDevice::draw(uint32 vertexCount, uint32 instanceCount /*= 1*/, uint32 firstVertex /*= 1*/, uint32 firstInstance /*= 1*/)
	{
	}

	void VulkanDevice::present()
	{
		//vk::vkCmdEndRendering(m_frame.m_commandBuffer);
		VK_CHECK(vk::vkEndCommandBuffer(m_frame.m_commandBuffer));

		m_swapchain->present(m_frame, m_graphicsQueue);

		VK_CHECK(vk::vkDeviceWaitIdle(m_device));

		resetFrame(m_frame);
	}

	uint32 VulkanDevice::getMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags flags)
	{
		for (uint32_t i = 0; i < m_memoryProperties.memoryTypeCount; ++i)
			if ((memoryTypeBits & (1 << i)) != 0 && (m_memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
				return i;

		LB_ERROR("No compatible memory type found");
		return ~0u;
	}

	void VulkanDevice::submitPipelineBarrier(const VkDependencyInfo& info)
	{
		vk::vkCmdPipelineBarrier2(m_frame.m_commandBuffer, &info);
	}

	void VulkanDevice::findPhysicalDevice()
	{
		uint32 gpuCount;
		vk::vkEnumeratePhysicalDevices(m_instance, &gpuCount, nullptr);
		ensure(gpuCount > 0);
		std::vector<VkPhysicalDevice> gpus(gpuCount);
		vk::vkEnumeratePhysicalDevices(m_instance, &gpuCount, gpus.data());

		struct Device
		{
			uint32 deviceIndex;
			uint32 score;
		};
		Device device = {};
		for (uint32 i = 0; i < gpuCount; ++i)
		{
			VkPhysicalDeviceProperties properties;
			vk::vkGetPhysicalDeviceProperties(gpus[i], &properties);

			uint32 score = 0;
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				score += 2;
			else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
				score += 1;
			// #note: atm I don't want to support any other device type, maybe in the future

			// prefer NVIDIA since I don't own an AMD card, so I can't test it
			if (properties.vendorID == 0x10de) // NVIDIA vendor ID
				score += 2;

			if (score > device.score)
			{
				device.deviceIndex = i;
				device.score = score;
			}
		}

		m_gpu = gpus[device.deviceIndex];
		
		// Get queue family indices
		uint32_t queueFamiliesCount = 0;
		vk::vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &queueFamiliesCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamiliesCount);
		vk::vkGetPhysicalDeviceQueueFamilyProperties(m_gpu, &queueFamiliesCount, queueFamilies.data());
		for (uint32 i = 0; i < queueFamiliesCount; ++i)
		{
			const VkQueueFamilyProperties& queueFamilyProperties = queueFamilies[i];

			// https://www.khronos.org/assets/uploads/developers/library/2016-vulkan-devday-uk/9-Asynchonous-compute.pdf
			// #important: since the main _test_ device is an nvidia card and according to the the previous .pdf
			// nvidia cards always have a queue family that supports all operations and has 16 queues,
            // I'm just going to use that queue family for now, note that with an AMD card it won't be possible
            // to create multiple queues from this queue family because the queue family that supports all operations only has 1 queue
			if (queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
				queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT  &&
				queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				m_graphicsQueueFamily = i;
				m_computeQueueFamily  = i;
				m_transferQueueFamily = i;
				break;
			}
		}

		ensure(m_graphicsQueueFamily != ~0);
		ensure(m_computeQueueFamily  != ~0);
		ensure(m_transferQueueFamily != ~0);

		VkPhysicalDeviceProperties properties;
		vk::vkGetPhysicalDeviceProperties(m_gpu, &properties);
		uint32 major, minor;
		if (properties.vendorID == 0x10de)
		{
			major = (properties.driverVersion >> 22) & 0x3ff;
			minor = (properties.driverVersion >> 14) & 0x0ff;
		}
		else
		{
			major = VK_API_VERSION_MAJOR(properties.driverVersion);
			minor = VK_API_VERSION_MINOR(properties.driverVersion);
		}

		LB_LOG("Initiliazed vulkan on %s using driver version %d.%d", properties.deviceName, major, minor);
	}
}
