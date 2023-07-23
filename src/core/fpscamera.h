#pragma once

#include "math.h"

struct GLFWwindow;

namespace limbo::core
{
	struct FPSCamera
	{
		float2 lastMousePos;

		float3 eye;
		float3 center;
		float3 up;

		float4x4 view;
		float4x4 proj;
		float4x4 viewProj;

		float4x4 prevView;
		float4x4 prevProj;
		float4x4 prevViewProj;
	};

	FPSCamera CreateCamera(const float3& eye, const float3& center);
	void UpdateCamera(GLFWwindow* window, FPSCamera& fpsCamera, float deltaTime);
}
