#pragma once

#include "shaderinterop.h"
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

		float		FOV   = 60.0f; // in degrees
		float		NearZ = 0.1f;
		float		FarZ  = 1000.0f;
		float		CameraSpeed = 0.5f;

		float4x4	View;
		float4x4	Proj;
		float4x4	ViewProj;

		float4x4	CascadeProjections[SHADOWMAP_CASCADES];

		float4x4	PrevView;
		float4x4	PrevProj;
		float4x4	PrevViewProj;

		void OnResize(uint32 width, uint32 height);
	};

	FPSCamera CreateCamera(const float3& eye, const float3& center);
	void UpdateCamera(Core::Window* window, FPSCamera& fpsCamera, float deltaTime);
}
