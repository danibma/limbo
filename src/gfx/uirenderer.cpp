#include "stdafx.h"
#include "uirenderer.h"

#include "scenerenderer.h"
#include "core/window.h"
#include "rhi/device.h"

namespace limbo::UI
{
	void Render(Gfx::SceneRenderer* sceneRenderer, float dt)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("Load"))
				{
					std::vector<wchar_t*> results;
					if (Utils::OpenFileDialog(sceneRenderer->Window, L"Choose scene to load", results, L"", { L"*.gltf; *.glb" }))
					{
						char path[MAX_PATH];
						Utils::StringConvert(results[0], path);
						sceneRenderer->LoadNewScene(path);
					}
				}

				if (ImGui::MenuItem("Clear Scene", "", false, sceneRenderer->HasScenes()))
					sceneRenderer->ClearScenes();

				ImGui::SeparatorText("Environment Map");
				{
					std::string envPreviewValue = sceneRenderer->EnvironmentMaps[sceneRenderer->Tweaks.SelectedEnvMapIdx].stem().string();
					if (ImGui::BeginCombo("##env_map", envPreviewValue.c_str()))
					{
						for (int i = 0; i < sceneRenderer->EnvironmentMaps.size(); i++)
						{
							const bool bIsSelected = (sceneRenderer->Tweaks.SelectedEnvMapIdx == i);
							if (ImGui::Selectable(sceneRenderer->EnvironmentMaps[i].stem().string().c_str(), bIsSelected))
							{
								if (sceneRenderer->Tweaks.SelectedEnvMapIdx != i)
									sceneRenderer->bNeedsEnvMapChange = true;
								sceneRenderer->Tweaks.SelectedEnvMapIdx = i;
							}

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (bIsSelected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}


				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Renderer"))
			{
				if (Gfx::CanTakeGPUCapture() && ImGui::MenuItem("Take GPU Capture"))
					Gfx::TakeGPUCapture();

				if (Gfx::CanTakeGPUCapture() && ImGui::MenuItem("Open Last GPU Capture"))
					Gfx::OpenLastGPUCapture();

				if (ImGui::MenuItem("Reload Shaders", "Ctrl-R"))
					Gfx::ReloadShaders();

				ImGui::EndMenu();
			}

			char menuText[256];
			snprintf(menuText, 256, "Device: %s | CPU Time: %.2f ms (%.2f fps)", Gfx::GetGPUInfo().Name, dt, 1000.0f / dt);
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize(menuText).x) - 10.0f);
			ImGui::Text(menuText);

			ImGui::EndMainMenuBar();
		}

		ImGui::SetNextWindowBgAlpha(0.7f);
		ImGui::Begin("Limbo##debugwindow", nullptr);
		{
			if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushItemWidth(200.0f);
				ImGui::Combo("Render Path", &sceneRenderer->Tweaks.CurrentRenderPath, sceneRenderer->RenderPathList, (int)Gfx::RenderPath::MAX);
				ImGui::Combo("Tonemap", &sceneRenderer->Tweaks.CurrentTonemap, sceneRenderer->TonemapList, (int)Gfx::Tonemap::MAX);
				ImGui::Combo("Scene Views", &sceneRenderer->Tweaks.CurrentSceneView, sceneRenderer->SceneViewList, (int)Gfx::SceneView::MAX);
				ImGui::Checkbox("VSync", &sceneRenderer->Tweaks.bEnableVSync);
				ImGui::PopItemWidth();

				ImGui::SeparatorText("Ambient Occlusion");
				ImGui::Combo("Technique", &sceneRenderer->Tweaks.CurrentAOTechnique, sceneRenderer->AOList, (int)Gfx::AmbientOcclusion::MAX);
				ImGui::DragFloat("SSAO Radius", &sceneRenderer->Tweaks.SSAORadius, 0.1f, 0.0f, 1.0f);
				ImGui::DragFloat("SSAO Power", &sceneRenderer->Tweaks.SSAOPower, 0.1f, 0.0f, 2.0f);

				if (sceneRenderer->Tweaks.CurrentAOTechnique == (int)Gfx::AmbientOcclusion::RTAO)
					ImGui::DragInt("RTAO Samples", &sceneRenderer->Tweaks.RTAOSamples, 1, 0, 16);
			}

			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Position: %.1f, %.1f, %.1f", sceneRenderer->Camera.Eye.x, sceneRenderer->Camera.Eye.y, sceneRenderer->Camera.Eye.z);
				ImGui::PushItemWidth(150.0f);
				ImGui::DragFloat("Speed", &sceneRenderer->Camera.CameraSpeed, 0.1f, 0.1f);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Set to light pos"))
					sceneRenderer->Camera.Eye = sceneRenderer->Light.Position;
			}

			if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("Light Position", &sceneRenderer->Light.Position[0], 0.1f);
				ImGui::ColorEdit3("Light Color", &sceneRenderer->Light.Color[0]);
			}

			ImGui::End();
		}
	}
}
