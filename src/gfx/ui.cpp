#include "stdafx.h"
#include "ui.h"

#include <imgui/imgui.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_tables.cpp>
#include <imgui/imgui_widgets.cpp>
#include <imgui/backends/imgui_impl_dx12.cpp>
#include <imgui/backends/imgui_impl_glfw.cpp>

#include "profiler.h"
#include "rendercontext.h"
#include "rhi/device.h"
#include "core/paths.h"
#include "core/utils.h"
#include "renderer/renderer.h"

namespace limbo::UI
{
	void Render(Gfx::RenderContext& context, float dt)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("Load"))
				{
					std::vector<wchar_t*> results;
					if (Utils::OpenFileDialog(context.Window, L"Choose scene to load", results, L"", { L"*.gltf; *.glb" }))
					{
						char path[MAX_PATH];
						Utils::StringConvert(results[0], path);
						context.LoadNewScene(path);
					}
				}

				if (ImGui::MenuItem("Clear Scene", "", false, context.HasScenes()))
					context.ClearScenes();

				ImGui::SeparatorText("Environment Map");
				{
					char envPreviewValue[1024];
					Paths::GetFilename(context.EnvironmentMaps[context.Tweaks.SelectedEnvMapIdx], envPreviewValue);
					if (ImGui::BeginCombo("##env_map", envPreviewValue))
					{
						for (uint32 i = 0; i < context.EnvironmentMaps.GetSize(); i++)
						{
							const bool bIsSelected = (context.Tweaks.SelectedEnvMapIdx == i);
							char selectedValue[1024];
							Paths::GetFilename(context.EnvironmentMaps[i], selectedValue);
							if (ImGui::Selectable(selectedValue, bIsSelected))
							{
								if (context.Tweaks.SelectedEnvMapIdx != i)
									context.bNeedsEnvMapChange = true;
								context.Tweaks.SelectedEnvMapIdx = i;
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
				if (RHI::CanTakeGPUCapture() && ImGui::MenuItem("Take GPU Capture"))
					RHI::TakeGPUCapture();

				if (RHI::CanTakeGPUCapture() && ImGui::MenuItem("Open Last GPU Capture"))
					RHI::OpenLastGPUCapture();

				if (ImGui::MenuItem("Reload Shaders", "Ctrl-R"))
					RHI::ReloadShaders();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Windows"))
			{
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				ImGui::Checkbox("Show Profiler", &Globals::bShowProfiler);
				ImGui::Checkbox("Show Shadow Maps Debug", &Globals::bDebugShadowMaps);
				ImGui::PopStyleVar();

				ImGui::EndMenu();
			}

			char menuText[256];
			snprintf(menuText, 256, "Device: %s | CPU Time: %.2f ms (%.2f fps) | GPU Time: %.2f ms", RHI::GetGPUInfo().Name, GCPUProfiler.GetRenderTime(), 1000.0f / GCPUProfiler.GetRenderTime(), GGPUProfiler.GetRenderTime());
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

				// Select which renderer to use
				// TODO: implement this in here RHI::GetGPUInfo().bSupportsRaytracing
				const auto& rendererNames = Gfx::Renderer::GetNames();
				int32 selectedRenderer = (int32)(std::find(rendererNames.cbegin(), rendererNames.cend(), context.CurrentRendererString) - rendererNames.cbegin());
				int32 currentRenderer = selectedRenderer;
				std::string rendererString;
				for (auto& i : rendererNames)
				{
					rendererString += i;
					rendererString += '\0';
				}

				if (ImGui::Combo("Renderer", &selectedRenderer, rendererString.c_str()))
				{
					if (currentRenderer != selectedRenderer)
					{
						context.CurrentRendererString = rendererNames[selectedRenderer];
						context.bUpdateRenderer = true;
					}
				}
				ImGui::Combo("Scene Views", &context.Tweaks.CurrentSceneView, context.SceneViewList, ENUM_COUNT<Gfx::SceneView>());
				ImGui::Checkbox("VSync", &context.Tweaks.bEnableVSync);
				ImGui::PopItemWidth();

				if (ImGui::CollapsingHeader("Render Options"))
				{
					for (auto& [name, value] : context.CurrentRenderOptions.Options)
					{
						if (std::holds_alternative<bool>(value))
						{
							ImGui::Checkbox(name.data(), &std::get<bool>(value));
						}
					}
				}

				int maxAO = (int)Gfx::AmbientOcclusion::MAX;
				if (!RHI::GetGPUInfo().bSupportsRaytracing)
					maxAO = (int)Gfx::AmbientOcclusion::RTAO;

				ImGui::SeparatorText("Ambient Occlusion");
				ImGui::Combo("Technique", &context.Tweaks.CurrentAOTechnique, context.AOList, maxAO);
				ImGui::DragFloat("SSAO Radius", &context.Tweaks.SSAORadius, 0.1f, 0.0f, 1.0f);
				ImGui::DragFloat("SSAO Power", &context.Tweaks.SSAOPower, 0.1f, 0.0f, 2.0f);

				if (context.Tweaks.CurrentAOTechnique == (int)Gfx::AmbientOcclusion::RTAO)
					ImGui::DragInt("RTAO Samples", &context.Tweaks.RTAOSamples, 1, 0, 16);

				ImGui::SeparatorText("Shadows");
				ImGui::Checkbox("Sun Casts Shadows", &context.Tweaks.bSunCastsShadows);
			}

			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Position: %.1f, %.1f, %.1f", context.Camera.Eye.x, context.Camera.Eye.y, context.Camera.Eye.z);
				ImGui::PushItemWidth(150.0f);
				ImGui::DragFloat("Speed", &context.Camera.CameraSpeed, 0.1f, 0.1f);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Set to light pos"))
					context.Camera.Eye = context.Light.Position;
			}

			if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("Light Position", &context.Light.Position[0], 0.1f);
				ImGui::ColorEdit3("Light Color", &context.Light.Color[0]);
			}

			if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("Direction", &context.Sun.Direction[0], 0.1f);
			}

			ImGui::End();
		}
	}
}
