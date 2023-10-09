#pragma once

// STL
#include <string>
#include <queue>
#include <vector>
#include <unordered_map>
#include <set>
#include <chrono>

// Third Party
#include <d3d12/d3d12.h>
#include <dxgi1_6.h>
#include <d3d12/d3dx12/d3dx12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#define D3DX12_NO_STATE_OBJECT_HELPERS
#include <d3d12/d3dx12/d3dx12.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <CppDelegates/Delegates.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#if !NO_LOG
#	define USE_PIX
#endif
#include <WinPixEventRuntime/pix3.h>

// Own stuff
#include "core/core.h"
#include "core/timer.h"
#include "core/utils.h"
#include "core/math.h"
#include "core/algo.h"
#include "gfx/rhi/definitions.h"
#include "core/refcountptr.h"
