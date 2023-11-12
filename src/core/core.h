#pragma once

#include <stdint.h>
#include <stdio.h>

#include "commandline.h"
#include "windowsplatform.h"

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
#define PLATFORM_BREAK() if (IsDebuggerPresent()) __debugbreak()

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

//
// Logging macros
//
#if !LB_RELEASE
	enum class InternalLogColor : uint8
	{
		Info  = 15, // FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
		Warn  = 14, // FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
		Error = 12  // FOREGROUND_RED | FOREGROUND_INTENSITY
	};

	template<typename CharType, typename... Args>
	FORCEINLINE void Internal_Log(const char* severity, InternalLogColor color, const CharType* format, Args&&... args)
	{
		constexpr uint16 bufferSize = 1024;
		CharType header[bufferSize], body[bufferSize];
		SetConsoleTextAttribute(limbo::Core::CommandLine::ConsoleHandle, (uint16)color);
		if constexpr (TIsSame<CharType, char>::Value)
		{
			snprintf(header, bufferSize, format, args...);
			snprintf(body, bufferSize, "[Limbo] %s: %s\n", severity, header);
			printf("%s", body);
			OutputDebugStringA(body);
		}
		else if constexpr (TIsSame<CharType, wchar_t>::Value)
		{
			_snwprintf_s(header, bufferSize, format, args...);
			_snwprintf_s(body, bufferSize, L"[Limbo] %s: %ls\n", severity, header);
			printf("%ls", body);
			OutputDebugStringW(body);
		}

		if (color == InternalLogColor::Error && !IsDebuggerPresent())
		{
			if constexpr (TIsSame<CharType, char>::Value)
				MessageBoxA(nullptr, body, "limbo", MB_OK);
			else if constexpr (TIsSame<CharType, wchar_t>::Value)
				MessageBoxW(nullptr, body, L"limbo", MB_OK);
			abort();
		}
	}

	#define LB_LOG(msg, ...) Internal_Log("Info", InternalLogColor::Info, msg, __VA_ARGS__)
	#define LB_WARN(msg, ...) Internal_Log("Warn", InternalLogColor::Warn, msg, __VA_ARGS__)
	#define LB_ERROR(msg, ...) do { Internal_Log("Error", InternalLogColor::Error, msg, __VA_ARGS__); PLATFORM_BREAK(); } while(0)

#else
	#define LB_LOG(msg, ...) __noop()
	#define LB_WARN(msg, ...) __noop()
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
		} \
	} while(0)

#define ENSURE_RETURN(expr, ...) if (!ensure(!(expr))) return __VA_ARGS__;

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
	constexpr FORCEINLINE T DivideAndRoundUp(T dividend, T divisor)
	{
		return (dividend + divisor - 1) / divisor;
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