#include "fpscamera.h"

#include "gfx/rhi/device.h"

#include "core/input.h"

namespace limbo::gfx
{
	FPSCamera createCamera(const float3& eye, const float3& center)
	{
		FPSCamera fpsCamera = {};

		const float aspect_ratio = (float)gfx::getBackbufferWidth() / (float)gfx::getBackbufferHeight();

		fpsCamera.eye = eye;
		fpsCamera.center = center;
		fpsCamera.up = float3(0.0f, 1.0f, 0.0f);

		fpsCamera.view = glm::lookAt(eye, center, fpsCamera.up);
		fpsCamera.proj = glm::perspective(0.6f, aspect_ratio, 1e-1f, 1e4f);
		fpsCamera.viewProj = fpsCamera.proj * fpsCamera.view;

		fpsCamera.prevView = fpsCamera.view;
		fpsCamera.prevProj = fpsCamera.proj;
		fpsCamera.prevViewProj = fpsCamera.viewProj;

		return fpsCamera;
	}

	void updateCamera(core::Window* window, FPSCamera& fpsCamera, float deltaTime)
	{
		// Update camera history
		fpsCamera.prevView = fpsCamera.view;
		fpsCamera.prevProj = fpsCamera.proj;

		// Update projection aspect ratio
		const float aspect_ratio = (float)gfx::getBackbufferWidth() / (float)gfx::getBackbufferHeight();

		fpsCamera.proj = glm::perspective(45.0f, aspect_ratio, 1e-1f, 1e4f);

		// Update view
		float cameraSpeed = fpsCamera.cameraSpeed * deltaTime;

		// Keyboard input
		float3 rightDirection = glm::cross(fpsCamera.center, fpsCamera.up);
		if (input::isKeyDown(window, input::KeyCode::W)) // W
			fpsCamera.eye += fpsCamera.center * cameraSpeed;
		if (input::isKeyDown(window, input::KeyCode::S)) // S
			fpsCamera.eye -= fpsCamera.center * cameraSpeed;
		if (input::isKeyDown(window, input::KeyCode::A)) // A
			fpsCamera.eye -= rightDirection * cameraSpeed;
		if (input::isKeyDown(window, input::KeyCode::D)) // D
			fpsCamera.eye += rightDirection * cameraSpeed;
		if (input::isKeyDown(window, input::KeyCode::LeftShift)) // Shift
			fpsCamera.eye -= fpsCamera.up * cameraSpeed;
		if (input::isKeyDown(window, input::KeyCode::Space)) // Spacebar
			fpsCamera.eye += fpsCamera.up * cameraSpeed;

		// mouse input
		float2 mousePos = input::getMousePos(window);
		float2 delta    = (mousePos - fpsCamera.lastMousePos) * 0.002f;
		fpsCamera.lastMousePos = mousePos;

		if (input::isMouseButtonDown(window, input::MouseButton::Right))
		{
			// Rotation
			if (delta.x != 0.0f || delta.y != 0.0f)
			{
				constexpr float rotation_speed = 1.5f;
				float pitchDelta = delta.y * rotation_speed;
				float yawDelta = delta.x * rotation_speed;

				glm::quat q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, rightDirection), glm::angleAxis(-yawDelta, float3(0.f, 1.0f, 0.0f))));
				fpsCamera.center = glm::rotate(q, fpsCamera.center);
			}
		}

		fpsCamera.view = glm::lookAt(fpsCamera.eye, fpsCamera.eye + fpsCamera.center, fpsCamera.up);

		// Update view projection matrix
		fpsCamera.viewProj = fpsCamera.proj * fpsCamera.view;
		fpsCamera.prevViewProj = fpsCamera.prevProj * fpsCamera.prevView;
	}

}

