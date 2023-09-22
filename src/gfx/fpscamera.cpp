#include "stdafx.h"
#include "fpscamera.h"

#include "gfx/rhi/device.h"

#include "core/input.h"

namespace limbo::Gfx
{
	void FPSCamera::OnResize(uint32 width, uint32 height)
	{
		// Update projection aspect ratio
		const float aspectRatio = (float)width / (float)height;

		Proj    = glm::perspective(glm::radians(FOV), aspectRatio, NearZ, FarZ);
		RevProj = Math::InfReversedProj_RH(glm::radians(FOV), aspectRatio, NearZ);

		// update shadow map cascade projections
		CascadeProjections[0] = glm::perspective(glm::radians(FOV), aspectRatio, NearZ , 30.0f);
		CascadeProjections[1] = glm::perspective(glm::radians(FOV), aspectRatio, 30.0f , 80.0f);
		CascadeProjections[2] = glm::perspective(glm::radians(FOV), aspectRatio, 80.0f , 400.0f);
		CascadeProjections[3] = glm::perspective(glm::radians(FOV), aspectRatio, 400.0f, FarZ);
	}

	FPSCamera CreateCamera(const float3& eye, const float3& center)
	{
		FPSCamera fpsCamera = {};

		const float aspectRatio = (float)RHI::GetBackbufferWidth() / (float)RHI::GetBackbufferHeight();

		fpsCamera.Eye = eye;
		fpsCamera.Center = center;
		fpsCamera.Up = float3(0.0f, 1.0f, 0.0f);

		fpsCamera.View = glm::lookAt(eye, center, fpsCamera.Up);
		fpsCamera.Proj = glm::perspective(glm::radians(fpsCamera.FOV), aspectRatio, fpsCamera.NearZ, fpsCamera.FarZ);
		fpsCamera.RevProj = Math::InfReversedProj_RH(glm::radians(fpsCamera.FOV), aspectRatio, fpsCamera.NearZ);
		fpsCamera.ViewProj = fpsCamera.Proj * fpsCamera.View;
		fpsCamera.ViewRevProj = fpsCamera.RevProj * fpsCamera.View;

		// Create shadow map cascade projections
		fpsCamera.CascadeProjections[0] = glm::perspective(glm::radians(fpsCamera.FOV), aspectRatio, fpsCamera.NearZ, 30.0f);
		fpsCamera.CascadeProjections[1] = glm::perspective(glm::radians(fpsCamera.FOV), aspectRatio, 30.0f			, 80.0f);
		fpsCamera.CascadeProjections[2] = glm::perspective(glm::radians(fpsCamera.FOV), aspectRatio, 80.0f			, 400.0f);
		fpsCamera.CascadeProjections[3] = glm::perspective(glm::radians(fpsCamera.FOV), aspectRatio, 400.0f			, fpsCamera.FarZ);

		return fpsCamera;
	}

	void UpdateCamera(Core::Window* window, FPSCamera& fpsCamera, float deltaTime)
	{
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
		fpsCamera.ViewRevProj = fpsCamera.RevProj * fpsCamera.View;
		fpsCamera.ViewProj = fpsCamera.Proj * fpsCamera.View;
	}

}

