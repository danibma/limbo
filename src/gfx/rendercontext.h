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
	namespace UIGlobals
	{
		inline bool bShowProfiler = false;
		inline bool bOrderProfilerResults = false;
		inline bool bDebugShadowMaps = false;
		inline int  ShadowCascadeIndex = 0;
		inline bool bShowShadowCascades = false;
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

	class Scene;
	class RenderContext
	{
		using EnvironmentMapList = TStaticArray<const char*, 7>;

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

			/**
			 * External textures are not created in the normal @CreateSceneTextures.
			 * These textures are created and deleted by render techniques or from other place in code.
			 * They should not be deleted in the normal @DestroySceneTextures.
			 */
			RHI::TextureHandle ShadowMaps[SHADOWMAP_CASCADES]; /**< External */
			RHI::TextureHandle AOTexture; /**< External */

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
		RenderTechniquesList			CurrentRenderTechniques;
		std::unique_ptr<Renderer>		CurrentRenderer;
		std::string_view				CurrentRendererString;
		FPSCamera						Camera;
		PointLight						Light;
		DirectionalLight				Sun;
		SceneTextures					SceneTextures;
		SceneInfo						SceneInfo;
		ShadowData						ShadowMapData;
		RHI::AccelerationStructure		SceneAccelerationStructure;

		bool							bUpdateRenderer = false;
		bool							bNeedsEnvMapChange = true;
		EnvironmentMapList				EnvironmentMaps;

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

		bool CanRenderSSAO() const;
		bool CanRenderRTAO() const;
		bool CanRenderShadows() const;
		bool IsAOEnabled() const;

	private:
		void LoadEnvironmentMap(RHI::CommandContext* cmd, const char* path);
		void UploadScenesToGPU();
		void UpdateSceneInfo();
		void UpdateRenderer();
		void CreateSceneTextures(uint32 width, uint32 height);
		void UpdateSceneTextures(uint32 width, uint32 height);
		void DestroySceneTextures();
		void RenderUI(float dt);
	};

	inline RenderContext* CreateSceneRenderer(Core::Window* window) { return new RenderContext(window); }
	inline void DestroySceneRenderer(RenderContext* sceneRenderer) { delete sceneRenderer; }
}
