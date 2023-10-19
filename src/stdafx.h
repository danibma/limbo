#pragma once

// STL
#include <string>
#include <queue>
#include <vector>
#include <unordered_map>
#include <set>
#include <mutex>
#include <format>

// Third Party
#include <d3d12/d3d12.h>
#define D3DX12_NO_STATE_OBJECT_HELPERS
#include <d3d12/d3dx12/d3dx12.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <CppDelegates/Delegates.h>
#if !LB_RELEASE
#	define USE_PIX
#endif
#include <WinPixEventRuntime/pix3.h>

#include "core/math.h"
#include "core/array.h"
#include "core/refcountptr.h"
