#include "stdafx.h"
#include "fpscamera.h"

#include "gfx/rhi/device.h"

#include "core/input.h"

namespace limbo::Gfx
{
	FPSCamera CreateCamera(const float3& eye, const float3& center)
	{
		FPSCamera fpsCamera = {};

		const float aspectRatio = (float)Gfx::GetBackbufferWidth() / (float)Gfx::GetBackbufferHeight();

		fpsCamera.Eye = eye;
		fpsCamera.Center = center;
		fpsCamera.Up = float3(0.0f, 1.0f, 0.0f);

		fpsCamera.View = glm::lookAt(eye, center, fpsCamera.Up);
		fpsCamera.Proj = glm::perspective(0.6f, aspectRatio, 1e-1f, 1e4f);
		fpsCamera.ViewProj = fpsCamera.Proj * fpsCamera.View;

		fpsCamera.PrevView		= fpsCamera.View;
		fpsCamera.PrevProj		= fpsCamera.Proj;
		fpsCamera.PrevViewProj	= fpsCamera.ViewProj;

		return fpsCamera;
	}

	void UpdateCamera(Core::Window* window, FPSCamera& fpsCamera, float deltaTime)
	{
		// Update camera history
		fpsCamera.PrevView = fpsCamera.View;
		fpsCamera.PrevProj = fpsCamera.Proj;

		// Update projection aspect ratio
		const float aspectRatio = (float)Gfx::GetBackbufferWidth() / (float)Gfx::GetBackbufferHeight();

		fpsCamera.Proj = glm::perspective(45.0f, aspectRatio, 1e-3f, 1e4f);

		fpsCamera.CameraSpeed += Input::GetScrollY(window) / 10.0f;
		if (fpsCamera.CameraSpeed <= 0.0f) fpsCamera.CameraSpeed = 0.1f;

		// Update view
		float cameraSpeed = (fpsCamera.CameraSpeed / 100.0f) * deltaTime;

		// Keyboard input
		float3 rightDirection = glm::cross(fpsCamera.Center, fpsCamera.Up);
		if (Input::IsKeyDown(window, Input::KeyCode::W)) // W
			fpsCamera.Eye += fpsCamera.Center * cameraSpeed;
		if (Input::IsKeyDown(window, Input::KeyCode::S)) // S
			fpsCamera.Eye -= fpsCamera.Center * cameraSpeed;
		if (Input::IsKeyDown(window, Input::KeyCode::A)) // A
			fpsCamera.Eye -= rightDirection * cameraSpeed;
		if (Input::IsKeyDown(window, Input::KeyCode::D)) // D
			fpsCamera.Eye += rightDirection * cameraSpeed;
		if (Input::IsKeyDown(window, Input::KeyCode::LeftShift)) // Shift
			fpsCamera.Eye -= fpsCamera.Up * cameraSpeed;
		if (Input::IsKeyDown(window, Input::KeyCode::Space)) // Spacebar
			fpsCamera.Eye += fpsCamera.Up * cameraSpeed;

		// mouse input
		float2 mousePos = Input::GetMousePos(window);
		float2 delta    = (mousePos - fpsCamera.LastMousePos) * 0.002f;
		fpsCamera.LastMousePos = mousePos;

		if (Input::IsMouseButtonDown(window, Input::MouseButton::Right))
		{
			// Rotation
			if (delta.x != 0.0f || delta.y != 0.0f)
			{
				constexpr float rotationSpeed = 1.5f;
				float pitchDelta = delta.y * rotationSpeed;
				float yawDelta = delta.x * rotationSpeed;

				glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, rightDirection), glm::angleAxis(-yawDelta, float3(0.f, 1.0f, 0.0f))));
				fpsCamera.Center = glm::rotate(q, fpsCamera.Center);
			}
		}

		fpsCamera.View = glm::lookAt(fpsCamera.Eye, fpsCamera.Eye + fpsCamera.Center, fpsCamera.Up);

		// Update view projection matrix
		fpsCamera.ViewProj = fpsCamera.Proj * fpsCamera.View;
		fpsCamera.PrevViewProj = fpsCamera.PrevProj * fpsCamera.PrevView;
	}

}

