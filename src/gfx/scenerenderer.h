#pragma once

#include <filesystem>

#include "fpscamera.h"
#include "shaderinterop.h"
#include "core/window.h"
#include "rhi/accelerationstructure.h"
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

	enum class RenderPath : uint8
	{
		Deferred = 0,
		PathTracing,

		MAX
	};

	enum class AmbientOcclusion : uint8
	{
		None = 0,
		SSAO,
		RTAO,

		MAX
	};

	struct RendererTweaks
	{
		bool		bEnableVSync		= true;

		// Ambient Occlusion
		float		SSAORadius			= 0.3f;
		float		SSAOPower			= 1.2f;
		int			RTAOSamples			= 2;

		// Scene
		int			CurrentTonemap		= 1; // Tonemap enum
		int			CurrentSceneView	= 0; // SceneView enum
		int			CurrentRenderPath	= 0; // RenderPath enum
		int			CurrentAOTechnique  = 1; // AmbientOcclusion enum
		int			SelectedEnvMapIdx	= 1;
	};

	struct PointLight
	{
		float3 Position;
		float3 Color;
	};

	struct DirectionalLight
	{
		float3 Direction;
	};

	class PathTracing;
	class Texture;
	class Buffer;
	class Shader;
	class Scene;
	class SSAO;
	class RTAO;
	class SceneRenderer
	{
		using EnvironmentMapList = std::vector<std::filesystem::path>;

	private:
		std::vector<Scene*>				m_Scenes;
		AccelerationStructure			m_SceneAS;
		Handle<Buffer>					m_SceneInfoBuffers[NUM_BACK_BUFFERS];

		Handle<Buffer>					m_ScenesMaterials;
		Handle<Buffer>					m_SceneInstances;

		Handle<Texture>					m_SceneTexture;

		Handle<Shader>					m_ShadowMapShader;

		// Skybox
		Handle<Shader>					m_SkyboxShader;
		Scene*							m_SkyboxCube;

		// Deferred shading
		Handle<Shader>					m_DeferredShadingShader;

		// IBL stuff
		Handle<Texture>					m_EnvironmentCubemap;
		Handle<Texture>					m_IrradianceMap;
		Handle<Texture>					m_PrefilterMap;
		Handle<Texture>					m_BRDFLUTMap;

		// PBR
		Handle<Shader>					m_PBRShader;

		// Scene Composite
		Handle<Shader>					m_CompositeShader;

		// Techniques
		std::unique_ptr<SSAO>			m_SSAO;
		std::unique_ptr<RTAO>			m_RTAO;
		std::unique_ptr<PathTracing>	m_PathTracing;

	public:
		Core::Window*				Window;

		RendererTweaks				Tweaks;
		FPSCamera					Camera;
		PointLight					Light;
		DirectionalLight			Sun;

		SceneInfo					SceneInfo;

		bool						bNeedsEnvMapChange = true;
		EnvironmentMapList			EnvironmentMaps;

		// This string lists are used for the UI
		const char*					TonemapList[(uint8)Tonemap::MAX]		= { "None", "AcesFilm", "Reinhard" };
		const char*					SceneViewList[(uint8)SceneView::MAX]	= { "Full", "Color", "Normal", "World Position", "Metallic", "Roughness", "Emissive", "Ambient Occlusion" };
		const char*					RenderPathList[(uint8)RenderPath::MAX]	= { "Deferred", "Path Tracing" };
		const char*					AOList[(uint8)AmbientOcclusion::MAX]	= { "None", "SSAO", "RTAO" };

	public:
		SceneRenderer(Core::Window* window);
		~SceneRenderer();

		// Update function with the time passed during that frame as a parameter, in ms
		void Render(float dt);

		void LoadNewScene(const char* path);
		void ClearScenes();
		bool HasScenes() const;

		void BindSceneInfo(Handle<Shader> shaderToBind);

	private:
		void LoadEnvironmentMap(const char* path);
		void UploadScenesToGPU();
		void UpdateSceneInfo();
	};

	inline SceneRenderer* CreateSceneRenderer(Core::Window* window) { return new SceneRenderer(window); }
	inline void DestroySceneRenderer(SceneRenderer* sceneRenderer) { delete sceneRenderer; }
}
