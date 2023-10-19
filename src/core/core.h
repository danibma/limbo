#pragma once

#include <stdint.h>
#include <stdio.h>

#include "commandline.h"

//
// General Macro
//
#if _DEBUG || !NDEBUG
	#define LIMBO_DEBUG 1
#else
	#define LIMBO_DEBUG 0
#endif

#define FORCENOINLINE __declspec(noinline)
#define FORCEINLINE   __forceinline

//
// Types
//
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

//
// Platform defines
//
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
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(_In_opt_ const char* lpOutputString);
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringW(_In_opt_ const wchar_t* lpOutputString);


//
// Logging macros
//
#define INTERNAL_PLATFORM_LOG(msg) OutputDebugStringA(msg)
#define INTERNAL_PLATFORM_WLOG(msg) OutputDebugStringW(msg)

#if !LB_RELEASE
	#define INTERNAL_PLATFORM_BREAK() __debugbreak();
#else
	#define INTERNAL_PLATFORM_BREAK();
#endif

#if !LB_RELEASE
	#define LB_LOG(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		char header[bufferSize], body[bufferSize]; \
		snprintf(header, bufferSize, msg, ##__VA_ARGS__); \
		snprintf(body, bufferSize, "[Limbo] Info: %s\n", header); \
		SetConsoleTextAttribute(limbo::Core::CommandLine::ConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); \
		printf("%s", body); \
		INTERNAL_PLATFORM_LOG(body); \
	}

	#define LB_WLOG(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		wchar_t header[bufferSize], body[bufferSize]; \
		_snwprintf_s(header, bufferSize, L##msg, ##__VA_ARGS__); \
		_snwprintf_s(body, bufferSize, L"[Limbo] Info: %ls\n", header); \
		SetConsoleTextAttribute(limbo::Core::CommandLine::ConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY); \
		printf("%ls", body); \
		INTERNAL_PLATFORM_WLOG(body); \
	}

	#define LB_WARN(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		char header[bufferSize], body[bufferSize]; \
		snprintf(header, bufferSize, msg, ##__VA_ARGS__); \
		snprintf(body, bufferSize, "[Limbo] Warn: %s\n", header); \
		SetConsoleTextAttribute(limbo::Core::CommandLine::ConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); \
		printf("%s", body); \
		INTERNAL_PLATFORM_LOG(body); \
	}

	#define LB_WWARN(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		wchar_t header[bufferSize], body[bufferSize]; \
		_snwprintf_s(header, bufferSize, L##msg, ##__VA_ARGS__); \
		_snwprintf_s(body, bufferSize, L"[Limbo] Warning: %ls\n", header); \
		SetConsoleTextAttribute(limbo::Core::CommandLine::ConsoleHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY); \
		printf("%ls", body); \
		INTERNAL_PLATFORM_WLOG(body); \
	}

	#define LB_ERROR(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		char header[bufferSize], body[bufferSize]; \
		snprintf(header, bufferSize, msg, ##__VA_ARGS__); \
		snprintf(body, bufferSize, "[Limbo] Error: %s\n", header); \
		SetConsoleTextAttribute(limbo::Core::CommandLine::ConsoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY); \
		printf("%s", body); \
		INTERNAL_PLATFORM_LOG(body); \
		INTERNAL_PLATFORM_BREAK(); \
	}

	#define LB_WERROR(msg, ...) \
	{ \
		constexpr uint16 bufferSize = 1024; \
		wchar_t header[bufferSize], body[bufferSize]; \
		_snwprintf_s(header, bufferSize, L##msg, ##__VA_ARGS__); \
		_snwprintf_s(body, bufferSize, L"[Limbo] Error: %ls\n", header); \
		SetConsoleTextAttribute(limbo::Core::CommandLine::ConsoleHandle, FOREGROUND_RED | FOREGROUND_INTENSITY); \
		printf("%ls", body); \
		INTERNAL_PLATFORM_WLOG(body); \
		INTERNAL_PLATFORM_BREAK(); \
	}
#else
	#define LB_LOG(msg, ...) __noop()
	#define LB_WLOG(msg, ...) __noop()
	#define LB_WWARN(msg, ...) __noop()
	#define LB_WARN(msg, ...) __noop()
	#define LB_WERROR(msg, ...) __noop()
	#define LB_ERROR(msg, ...) __noop()
#endif

//
// Assertion macros
//
#if !LB_RELEASE
	#define ensure(expr) \
		([&]() \
		{\
			bool r = (expr);\
			if (!(r)) \
				LB_ERROR("Assertion Failed!"); \
			return r; \
		}())
#else 
	#define ensure(expr) (expr)
#endif

#define check(expr) \
	do { \
		bool r = (expr); \
		if (!(r)) \
		{ \
			LB_ERROR("Check failed! '%s'", #expr); \
			abort(); \
		} \
	} while(0)

#define ENSURE_RETURN(expr, ...) if (!ensure(!(expr))) return __VA_ARGS__;

/* An empty function as breakpoint target
Usage: just put Noop(); at any line in a function, then you can set a breakpoint there.
*/
FORCEINLINE void Noop() {}

/* An empty function to reference a variable that is not used
Usage: just put Noop(<varName>); at any line in a function, then you can set a breakpoint there.
*/
template<typename... Args>
FORCEINLINE void Noop(Args... args) {}

#define LB_NON_COPYABLE(TYPE)  \
    TYPE(TYPE const &) = delete; TYPE &operator =(TYPE const &) = delete

//
// TEnableIf
//
template <bool Predicate, typename Result = void>
class TEnableIf;

template <typename Result>
class TEnableIf<true, Result>
{
public:
	using type = Result;
	using Type = Result;
};

template <typename Result>
class TEnableIf<false, Result>
{ };

//
// TIsEnum
//
template <typename T>
struct TIsEnum
{
	enum { Value = __is_enum(T) };
};

//
// TIsSame
//
template<typename A, typename B>	struct TIsSame { enum { Value = false }; };
template<typename T>				struct TIsSame<T, T> { enum { Value = true }; };

// Support bitwise operation
#define DECLARE_ENUM_BITWISE_OPERATORS(T) \
	inline constexpr T operator|(T a, T b) { return (T)((uint64_t)a | (uint64_t)b); } \
	inline constexpr T operator&(T a, T b) { return (T)((uint64_t)a & (uint64_t)b); } \
	inline constexpr T operator^(T a, T b) { return (T)((uint64_t)a ^ (uint64_t)b); } \
	inline constexpr T operator~(T a) { return (T)~(uint64_t)a; } \
	inline T& operator|=(T& a, T b) { return a = (T)((uint64_t)a | (uint64_t)b); } \
	inline T& operator&=(T& a, T b) { return a = (T)((uint64_t)a & (uint64_t)b); } \
	inline T& operator^=(T& a, T b) { return a = (T)((uint64_t)a ^ (uint64_t)b); }

template<typename Enum, typename = TEnableIf<TIsEnum<Enum>::Value>>
constexpr inline bool EnumHasAllFlags(Enum Flags, Enum Contains)
{
	return (((__underlying_type(Enum))Flags) & (__underlying_type(Enum))Contains) == ((__underlying_type(Enum))Contains);
}

template<typename Enum, typename = TEnableIf<TIsEnum<Enum>::Value>>
constexpr inline bool EnumHasAnyFlags(Enum Flags, Enum Contains)
{
	return (((__underlying_type(Enum))Flags) & (__underlying_type(Enum))Contains) != 0;
}

// Simple math operations
namespace limbo::Math
{
	template<typename T>
	constexpr FORCEINLINE T Align(T value, T alignement)
	{
		return (value + (alignement - 1)) & ~(alignement - 1);
	}

	template<typename T>
	constexpr FORCEINLINE T Max(T first, T second)
	{
		return first >= second ? first : second;
	}

	template<typename T>
	constexpr FORCEINLINE T Min(T first, T second)
	{
		return first <= second ? first : second;
	}

	template<typename T>
	constexpr FORCEINLINE T Abs(T value)
	{
		return value < 0 ? -value : value;
	}
}

template<typename T, size_t N>
constexpr FORCEINLINE uint32 ARRAY_LEN(T const(&)[N]) { return uint32(N); }

template<typename T>
constexpr FORCEINLINE uint8 ENUM_COUNT() { return uint8(T::MAX); }