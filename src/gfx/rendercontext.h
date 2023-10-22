#pragma once

#include "fpscamera.h"
#include "shaderinterop.h"
#include "core/window.h"
#include "renderer/renderer.h"
#include "rhi/accelerationstructure.h"
#include "rhi/texture.h"

namespace limbo::RHI
{
	class CommandContext;
}

namespace limbo::Gfx
{
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

	enum class AmbientOcclusion : uint8
	{
		None = 0,
		SSAO,
		RTAO, // Leave this as the last one, so it does not show in the UI as an option, if rt is not available

		MAX
	};

	struct RendererTweaks
	{
		bool				bEnableVSync		= true;

		bool				bSunCastsShadows	= true;

		// Ambient Occlusion
		float				SSAORadius			= 0.3f;
		float				SSAOPower			= 1.2f;
		int					RTAOSamples			= 2;

		int					CurrentSceneView	= 0; // SceneView enum
		int					CurrentAOTechnique  = 1; // AmbientOcclusion enum
		int					SelectedEnvMapIdx	= 1;
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
	class RenderContext
	{
		using EnvironmentMapList = TStaticArray<const char*, 8>;

		struct SceneTextures
		{
			RHI::TextureHandle PreCompositeSceneTexture;

			// GBuffer
			RHI::TextureHandle GBufferRenderTargetA; /**< PixelPosition */
			RHI::TextureHandle GBufferRenderTargetB; /**< WorldPosition */
			RHI::TextureHandle GBufferRenderTargetC; /**< Albedo */
			RHI::TextureHandle GBufferRenderTargetD; /**< Normal */
			RHI::TextureHandle GBufferRenderTargetE; /**< RoughnessMetallicAO */
			RHI::TextureHandle GBufferRenderTargetF; /**< Emissive */
			RHI::TextureHandle GBufferDepthTarget;

			RHI::TextureHandle AOTexture;

			/**
			 * External textures are not created in the normal @CreateSceneTextures.
			 * These textures are created and deleted by render techniques or from other place in code.
			 * They should not be deleted in the normal @DestroySceneTextures.
			 */
			RHI::TextureHandle DepthShadowMaps[SHADOWMAP_CASCADES]; /**< External */

			// IBL
			RHI::TextureHandle EnvironmentCubemap; /**< External */
			RHI::TextureHandle IrradianceMap; /**< External */
			RHI::TextureHandle PrefilterMap; /**< External */
			RHI::TextureHandle BRDFLUTMap; /**< External */
		};

	private:
		std::vector<Scene*>				m_Scenes;

		RHI::BufferHandle				m_ScenesMaterials;
		RHI::BufferHandle				m_SceneInstances;

	public:
		Core::Window*					Window;
		uint2							RenderSize;
		RenderOptions					CurrentRenderOptions;
		RenderTechniquesList			CurrentRenderTechniques;
		std::unique_ptr<Renderer>		CurrentRenderer;
		std::string_view				CurrentRendererString;
		RendererTweaks					Tweaks;
		FPSCamera						Camera;
		PointLight						Light;
		DirectionalLight				Sun;
		SceneTextures					SceneTextures;
		SceneInfo						SceneInfo;
		RHI::AccelerationStructure		SceneAccelerationStructure;

		bool							bUpdateRenderer = false;
		bool							bNeedsEnvMapChange = true;
		EnvironmentMapList				EnvironmentMaps;

		// This string lists are used for the UI
		const char*						SceneViewList[ENUM_COUNT<SceneView>()] = {"Full", "Color", "Normal", "World Position", "Metallic", "Roughness", "Emissive", "Ambient Occlusion"};
		const char*						AOList[ENUM_COUNT<AmbientOcclusion>()] = {"None", "SSAO", "RTAO"};

	public:
		RenderContext(Core::Window* window);
		~RenderContext();

		std::vector<RHI::TextureHandle> GetGBufferTextures() const;

		// Update function with the time passed during that frame as a parameter, in ms
		void Render(float dt);

		// Scene functions
		void LoadNewScene(const char* path);
		void ClearScenes();
		bool HasScenes() const;
		const std::vector<Scene*>& GetScenes() const;

	private:
		void LoadEnvironmentMap(RHI::CommandContext* cmd, const char* path);
		void UploadScenesToGPU();
		void UpdateSceneInfo();
		void UpdateRenderer();
		void CreateSceneTextures(uint32 width, uint32 height);
		void UpdateSceneTextures(uint32 width, uint32 height);
		void DestroySceneTextures();
	};

	inline RenderContext* CreateSceneRenderer(Core::Window* window) { return new RenderContext(window); }
	inline void DestroySceneRenderer(RenderContext* sceneRenderer) { delete sceneRenderer; }
}
