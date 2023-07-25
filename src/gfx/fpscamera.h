#pragma once

#include "core/math.h"

namespace limbo::core
{
	class Window;
}

namespace limbo::gfx
{
	struct FPSCamera
	{
		float2 lastMousePos;

		float3 eye;
		float3 center;
		float3 up;

		float cameraSpeed = 2.0f;

		float4x4 view;
		float4x4 proj;
		float4x4 viewProj;

		float4x4 prevView;
		float4x4 prevProj;
		float4x4 prevViewProj;
	};

	FPSCamera createCamera(const float3& eye, const float3& center);
	void updateCamera(core::Window* window, FPSCamera& fpsCamera, float deltaTime);
}
