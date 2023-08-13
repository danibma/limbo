#pragma once

#include <filesystem>

#include "fpscamera.h"
#include "core/window.h"
#include "rhi/resourcepool.h"

namespace limbo::Gfx
{
	enum class Tonemap : uint8
	{
		None = 0,
		AcesFilm,
		Reinhard,

		MAX
	};

	enum class SceneView : uint8
	{
		Full = 0,
		Color,
		Normal,
		WorldPosition,
		Metallic,
		Roughness,
		Emissive,
		AO,

		MAX
	};

	struct RendererTweaks
	{
		// SSAO
		bool		bEnableSSAO			= true;
		float		SSAORadius			= 0.3f;
		float		SSAOPower			= 1.2f;

		// Scene
		int			CurrentTonemap		= 1;
		int			CurrentSceneView	= 0;
		int			SelectedEnvMapIdx	= 4;
	};

	struct PointLight
	{
		float3 Position;
		float3 Color;
	};

	class Texture;
	class Buffer;
	class Shader;
	class Scene;
	class SceneRenderer
	{
		using EnvironmentMapList = std::vector<std::filesystem::path>;

	private:
		std::vector<Scene*>			m_Scenes;

		// Skybox
		Handle<Shader>				m_SkyboxShader;
		Scene*						m_SkyboxCube;

		// Deferred shading
		Handle<Shader>				m_DeferredShadingShader;

		// IBL stuff
		Handle<Texture>				m_EnvironmentCubemap;
		Handle<Texture>				m_IrradianceMap;
		Handle<Texture>				m_PrefilterMap;
		Handle<Texture>				m_BRDFLUTMap;

		// PBR
		Handle<Shader>				m_PBRShader;

		// Scene Composite
		Handle<Shader>				m_CompositeShader;

		// SSAO
		Handle<Shader>				m_SSAOShader;
		Handle<Texture>				m_UnblurredSSAOTexture;
		Handle<Shader>				m_BlurSSAOShader;
		Handle<Texture>				m_BlurredSSAOTexture;

	public:
		Core::Window*				Window;

		RendererTweaks				Tweaks;
		FPSCamera					Camera;
		PointLight					Light;

		uint32						FrameIndex;

		bool						bNeedsEnvMapChange = true;
		EnvironmentMapList			EnvironmentMaps;

		// This string lists are used for the UI
		const char*					TonemapList[(uint8)Tonemap::MAX]	 = { "None", "AcesFilm", "Reinhard" };
		const char*					SceneViewList[(uint8)SceneView::MAX] = { "Full", "Color", "Normal", "World Position", "Metallic", "Roughness", "Emissive", "Ambient Occlusion" };

	public:
		SceneRenderer(Core::Window* window);
		~SceneRenderer();

		// Update function with the time passed during that frame as a parameter, in ms
		void Render(float dt);

		void LoadNewScene(const char* path);
		void ClearScenes();
		bool HasScenes() const;

	private:
		void LoadEnvironmentMap(const char* path);
	};

	inline SceneRenderer* CreateSceneRenderer(Core::Window* window) { return new SceneRenderer(window); }
	inline void DestroySceneRenderer(SceneRenderer* sceneRenderer) { delete sceneRenderer; }
}
