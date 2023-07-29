#pragma once

//Containers
#include <string>
#include <queue>
#include <vector>
#include <unordered_map>

// Third Party
#include <d3d12/d3d12.h>
#include <dxgi1_6.h>
#include <d3d12/d3dx12/d3dx12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <dxgidebug.h>
#define D3DX12_NO_STATE_OBJECT_HELPERS
#include <d3d12/d3dx12/d3dx12.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <CppDelegates/Delegates.h>
#include <CLI11/CLI11.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

// Own stuff
#include "core/core.h"
#include "core/timer.h"
#include "core/utils.h"
#include "core/math.h"
#include "core/algo.h"
#include "gfx/rhi/definitions.h"
