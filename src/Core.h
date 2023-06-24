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
				INTERNAL_PLATFORM_BREAK();
			return expression;
		}
	}
	#define ensure(expr) limbo::internal::InternalEnsure(expr)
#else 
	#define ensure(expr) (expr)
#endif