#pragma once

#include "core/math.h"

namespace limbo::Core
{
	class Window;
}

namespace limbo::Gfx
{
	struct FPSCamera
	{
		float2		LastMousePos;

		float3		Eye;
		float3		Center;
		float3		Up;

		float		CameraSpeed = 2.0f;

		float4x4	View;
		float4x4	Proj;
		float4x4	ViewProj;

		float4x4	PrevView;
		float4x4	PrevProj;
		float4x4	PrevViewProj;
	};

	FPSCamera CreateCamera(const float3& eye, const float3& center);
	void UpdateCamera(Core::Window* window, FPSCamera& fpsCamera, float deltaTime);
}
