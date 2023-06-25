#pragma once

#include <utility>
#include <stdio.h>
#include <stdint.h>

#if _DEBUG || !NDEBUG
	#define LIMBO_DEBUG 1
#else
	#define LIMBO_DEBUG 0
#endif

//
// Types
//
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

//
// Logging macros
//
#if LIMBO_WINDOWS

	extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(_In_opt_ const char* lpOutputString);
	#define INTERNAL_PLATFORM_LOG(msg) OutputDebugStringA(msg)

	#if LIMBO_DEBUG
		#define INTERNAL_PLATFORM_BREAK() __debugbreak();
	#else
		#define INTERNAL_PLATFORM_BREAK();
	#endif

#elif LIMBO_LINUX
	#define INTERNAL_PLATFORM_LOG(msg)
	#define INTERNAL_PLATFORM_BREAK() __builtin_debugtrap()
#else
	#error Platform not implemented
#endif

#if LIMBO_DEBUG
	#define LB_LOG(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		char header[bufferSize], body[bufferSize]; \
		snprintf(header, bufferSize, msg, ##__VA_ARGS__); \
		snprintf(body, bufferSize, "[Limbo] Info: %s\n", header); \
		printf("%s", body); \
		INTERNAL_PLATFORM_LOG(body); \
	}

	#define LB_ERROR(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		char header[bufferSize], body[bufferSize]; \
		snprintf(header, bufferSize, msg, ##__VA_ARGS__); \
		snprintf(body, bufferSize, "[Limbo] Error: %s -> %s:%d\n", header, __FILE__, __LINE__); \
		printf("%s", body); \
		INTERNAL_PLATFORM_LOG(body); \
		INTERNAL_PLATFORM_BREAK(); \
	}
#else
	#define LB_LOG(msg, ...)
	#define LB_ERROR(msg, ...)
#endif

//
// Assertion macros
//
#if LIMBO_DEBUG
	namespace limbo::internal
	{
		inline bool InternalEnsure(bool expression)
		{
			if (!expression)
				LB_ERROR("Assertion Failed!");
			return expression;
		}
	}
	#define ensure(expr) limbo::internal::InternalEnsure(expr)
#else 
	#define ensure(expr) (expr)
#endif

template <typename T>
void Noop(T exp) {}

#include <vulkan/vulkan.h>

#if LIMBO_WINDOWS
#include <windows.h>
#elif LIMBO_LINUX
#include <dlfcn.h>
#endif

/*
typedef const char* LPCSTR;
typedef struct HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;

#ifdef _WIN64
typedef __int64(__stdcall* FARPROC)(void);
#else
typedef int(__stdcall* FARPROC)(void);
#endif

__declspec(dllimport) HMODULE __stdcall LoadLibraryA(LPCSTR);
__declspec(dllimport) FARPROC __stdcall GetProcAddress(HMODULE, LPCSTR);*/
/*

void vulkanInit()
{
	//PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	PFN_vkCreateInstance vkCreateInstance;

#if LIMBO_WINDOWS
	HMODULE module = LoadLibraryA("vulkan-1.dll");

	//vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)(void(*)(void))GetProcAddress(module, "vkGetInstanceProcAddr");

	vkCreateInstance = (PFN_vkCreateInstance)GetProcAddress(module, "vkCreateInstance");
#elif LIMBO_LINUX
	void* module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		module = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
	ensure(module);
	//vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
	vkCreateInstance = (PFN_vkCreateInstance)dlsym(module, "vkCreateInstance");
#endif

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
	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
	LB_LOG("%d", (int)result);
}*/