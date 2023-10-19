#include "stdafx.h"

#include "gfx/gfx.h"
#include "gfx/rhi/device.h"
#include "gfx/fpscamera.h"

#include "core/timer.h"
#include "core/window.h"
#include "core/commandline.h"
#include "core/jobsystem.h"
#include "gfx/profiler.h"

#include "tests/tests.h"

#include "gfx/scenerenderer.h"
#include "gfx/uirenderer.h"


using namespace limbo;

#define WIDTH  1280
#define HEIGHT 720

int WINAPI WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PSTR lpCmdLine, int nCmdShow)
{
	Core::Timer initTimer;

	Core::JobSystem::Initialize();

	Core::CommandLine::Init(lpCmdLine);
	if (Core::CommandLine::HasArg("--tests"))
		return Tests::ExecuteTests(lpCmdLine);

	if (Core::CommandLine::HasArg(LIMBO_CMD_WAIT_FOR_DEBUGGER))
	{
		while (!IsDebuggerPresent())
			;
	}

	Core::Window* window = Core::NewWindow({
		.Title = "limbo",
		.Width = WIDTH,
		.Height = HEIGHT
	});

	Gfx::Init(window, Gfx::GfxDeviceFlag::EnableImgui);

	Profiler::Initialize();

	Gfx::SceneRenderer* sceneRenderer = Gfx::CreateSceneRenderer(window);

	LB_LOG("Took %.2fs to initialize all the systems", initTimer.ElapsedSeconds());

	Core::Timer deltaTimer;
	while (!window->ShouldClose())
	{
		float deltaTime = deltaTimer.ElapsedMilliseconds();
		deltaTimer.Record();

		window->PollEvents();

		Gfx::UpdateCamera(window, sceneRenderer->Camera, deltaTime);

		bool ReloadShadersBind = Input::IsKeyDown(window, Input::KeyCode::LeftControl) && Input::IsKeyDown(window, Input::KeyCode::R);
		if (ReloadShadersBind)
			RHI::ReloadShaders();

		UI::Render(sceneRenderer, deltaTime);

		sceneRenderer->Render(deltaTime);

		Profiler::Present();
	}

	Gfx::DestroySceneRenderer(sceneRenderer);
	Profiler::Shutdown();
	Gfx::Shutdown();
	Core::DestroyWindow(window);

	return 0;
}