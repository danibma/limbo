#pragma once

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
extern "C" __declspec(dllimport) int __stdcall MessageBoxA(_In_opt_ HWND hWnd, _In_opt_ LPCSTR lpText, _In_opt_ LPCSTR lpCaption, _In_ UINT uType);
extern "C" __declspec(dllimport) int __stdcall MessageBoxW(_In_opt_ HWND hWnd, _In_opt_ LPCWSTR lpText, _In_opt_ LPCWSTR lpCaption, _In_ UINT uType);
