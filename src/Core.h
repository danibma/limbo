#pragma once

#include <stdint.h>
#include <stdio.h>

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
	extern "C" __declspec(dllimport) void __stdcall OutputDebugStringW(_In_opt_ const wchar_t* lpOutputString);
	#define INTERNAL_PLATFORM_LOG(msg) OutputDebugStringA(msg)
	#define INTERNAL_PLATFORM_WLOG(msg) OutputDebugStringW(msg)

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

#if !NO_LOG
	#define LB_LOG(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		char header[bufferSize], body[bufferSize]; \
		snprintf(header, bufferSize, msg, ##__VA_ARGS__); \
		snprintf(body, bufferSize, "[Limbo] Info: %s\n", header); \
		printf("%s", body); \
		INTERNAL_PLATFORM_LOG(body); \
	}

	#define LB_WLOG(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		wchar_t header[bufferSize], body[bufferSize]; \
		_snwprintf_s(header, bufferSize, L##msg, ##__VA_ARGS__); \
		_snwprintf_s(body, bufferSize, L"[Limbo] Info: %ls\n", header); \
		printf("%ls", body); \
		INTERNAL_PLATFORM_WLOG(body); \
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

	#define LB_WERROR(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		wchar_t header[bufferSize], body[bufferSize]; \
		_snwprintf_s(header, bufferSize, L##msg, ##__VA_ARGS__); \
		_snwprintf_s(body, bufferSize, L"[Limbo] Error: %ls -> %hs:%d\n", header, __FILE__, __LINE__); \
		printf("%ls", body); \
		INTERNAL_PLATFORM_WLOG(body); \
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
	#define ensure(expr) \
		([&]() \
		{\
			if (!(expr)) \
				LB_ERROR("Assertion Failed!"); \
			return expr; \
		}())
#else 
	#define ensure(expr) (expr)
#endif

#define FAILIF(expr) if (!ensure(!(expr))) return;

template<typename T>
void Noop(T exp) {}

//
// Platform defines
//
#if LIMBO_WINDOWS
	typedef struct HWND__* HWND;
	typedef const char* LPCSTR;
	typedef struct HINSTANCE__* HINSTANCE;
	typedef HINSTANCE HMODULE;
	typedef void* HANDLE;
	typedef const wchar_t* LPCWSTR;
	typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;
	typedef unsigned long DWORD;
	typedef struct HMONITOR__* HMONITOR;
	typedef long HRESULT;
	typedef long long INT_PTR;
	typedef INT_PTR(__stdcall* FARPROC)();

	extern "C" __declspec(dllimport) HMODULE __stdcall LoadLibraryA(LPCSTR lpLibFileName);
	extern "C" __declspec(dllimport) FARPROC __stdcall GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
#elif LIMBO_LINUX
	#include <dlfcn.h>
#endif
