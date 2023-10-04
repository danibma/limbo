#pragma once

#include <filesystem>

#include "fpscamera.h"
#include "shaderinterop.h"
#include "core/window.h"
#include "rhi/accelerationstructure.h"
#include "rhi/resourcepool.h"

namespace limbo::RHI
{
	struct Shader;
	class RootSignature;
	class PipelineStateObject;
	class Texture;
	class Buffer;
}

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

		bool		bSunCastsShadows	= true;

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

	class ShadowMapping;
	class PathTracing;
	class Scene;
	class SSAO;
	class RTAO;
	class SceneRenderer
	{
		using EnvironmentMapList = std::vector<std::filesystem::path>;

	private:
		std::vector<Scene*>				m_Scenes;
		RHI::AccelerationStructure		m_SceneAS;

		RHI::Handle<RHI::Buffer>		m_ScenesMaterials;
		RHI::Handle<RHI::Buffer>		m_SceneInstances;

		RHI::Handle<RHI::Texture>		m_SceneTexture;

		RHI::Handle<RHI::Shader>		m_QuadShaderVS;

		// Skybox
		RHI::RootSignature*				m_SkyboxRS;
		RHI::Handle<RHI::Shader>		m_SkyboxShaderVS;
		RHI::Handle<RHI::Shader>		m_SkyboxShaderPS;
		RHI::PipelineStateObject*		m_SkyboxPSO;
		Scene*							m_SkyboxCube;

		// Deferred shading
		RHI::RootSignature*				m_DeferredShadingRS;
		RHI::Handle<RHI::Shader>		m_DeferredShadingShaderVS;
		RHI::Handle<RHI::Shader>		m_DeferredShadingShaderPS;
		RHI::PipelineStateObject*		m_DeferredShadingPSO;
		RHI::Handle<RHI::Texture>		m_DeferredShadingRTs[6];
		RHI::Handle<RHI::Texture>		m_DeferredShadingDT;

		// IBL stuff
		RHI::Handle<RHI::Texture>		m_EnvironmentCubemap;
		RHI::Handle<RHI::Texture>		m_IrradianceMap;
		RHI::Handle<RHI::Texture>		m_PrefilterMap;
		RHI::Handle<RHI::Texture>		m_BRDFLUTMap;

		// PBR
		RHI::RootSignature*				m_LightingRS;
		RHI::Handle<RHI::Shader>		m_PBRShaderPS;
		RHI::PipelineStateObject*		m_PBRPSO;
		RHI::Handle<RHI::Texture>		m_LightingRT;

		// Scene Composite
		RHI::RootSignature*				m_CompositeRS;
		RHI::Handle<RHI::Shader>		m_CompositeShaderPS;
		RHI::PipelineStateObject*		m_CompositePSO;

		// Techniques
		std::unique_ptr<SSAO>			m_SSAO;
		std::unique_ptr<RTAO>			m_RTAO;
		std::unique_ptr<PathTracing>	m_PathTracing;
		std::unique_ptr<ShadowMapping>  m_ShadowMapping;
	public:
		Core::Window*					Window;

		RendererTweaks					Tweaks;
		FPSCamera						Camera;
		PointLight						Light;
		DirectionalLight				Sun;

		SceneInfo						SceneInfo;

		bool							bNeedsEnvMapChange = true;
		EnvironmentMapList				EnvironmentMaps;

		// This string lists are used for the UI
		const char*						TonemapList[(uint8)Tonemap::MAX]		= { "None", "AcesFilm", "Reinhard" };
		const char*						SceneViewList[(uint8)SceneView::MAX]	= { "Full", "Color", "Normal", "World Position", "Metallic", "Roughness", "Emissive", "Ambient Occlusion" };
		const char*						RenderPathList[(uint8)RenderPath::MAX]	= { "Deferred", "Path Tracing" };
		const char*						AOList[(uint8)AmbientOcclusion::MAX]	= { "None", "SSAO", "RTAO" };

	public:
		SceneRenderer(Core::Window* window);
		~SceneRenderer();

		// Render Passes
		void RenderGeometryPass();
		void RenderAmbientOcclusionPass();
		void RenderSkybox();
		void RenderLightingPass();
		void RenderSceneCompositePass();

		// Update function with the time passed during that frame as a parameter, in ms
		void Render(float dt);

		// Scene functions
		void LoadNewScene(const char* path);
		void ClearScenes();
		bool HasScenes() const;
		const std::vector<Scene*>& GetScenes() const;

	private:
		void LoadEnvironmentMap(const char* path);
		void UploadScenesToGPU();
		void UpdateSceneInfo();
	};

	inline SceneRenderer* CreateSceneRenderer(Core::Window* window) { return new SceneRenderer(window); }
	inline void DestroySceneRenderer(SceneRenderer* sceneRenderer) { delete sceneRenderer; }
}
