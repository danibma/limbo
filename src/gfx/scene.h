#pragma once

#include <functional> // for std::function

#include "core/math.h"

#include "rhi/resourcemanager.h"
#include "rhi/resourcepool.h"

struct cgltf_node;
struct cgltf_scene;
struct cgltf_mesh;
struct cgltf_primitive;
struct cgltf_texture_view;
struct cgltf_material;

namespace limbo::gfx
{
	class Texture;
	class Buffer;

	struct MeshVertex
	{
		float3 position;
		float2 uv;
	};

	struct MeshMaterial
	{
		Handle<Texture> albedo;
		Handle<Texture> roughnessMetal;
		Handle<Texture> normal;
		Handle<Texture> emissive;

		MeshMaterial()
		{
			albedo			= ResourceManager::ptr->emptyTexture;
			roughnessMetal	= ResourceManager::ptr->emptyTexture;
			normal			= ResourceManager::ptr->emptyTexture;
			emissive		= ResourceManager::ptr->emptyTexture;
		}
	};

	struct Mesh
	{
		Handle<Buffer>			vertexBuffer;
		Handle<Buffer>			indexBuffer;

		uintptr_t				materialID;

		float4x4				transform;

		size_t					indexCount = 0;
		size_t					vertexCount = 0;

		const char*				name;

		Mesh(const char* meshName, const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, uintptr_t material);
	};

	class Scene
	{
		std::vector<Mesh>								m_meshes;
		char											m_folderPath[256];
		char											m_sceneName[128];

		std::unordered_map<uintptr_t, MeshMaterial>	m_meshMaterials;

	protected:
		Scene() = default;
		Scene(const char* path);

	public:
		static Scene* load(const char* path);
		void destroy();

		void drawMesh(const std::function<void(const Mesh& mesh)>& drawFunction);
		MeshMaterial getMaterial(uintptr_t pMaterial);

	private:
		void processNode(const cgltf_node* node);
		void processMaterial(const cgltf_material* material);
		Mesh processMesh(const cgltf_mesh* mesh, const cgltf_primitive* primitive);

		void loadTexture(const cgltf_texture_view* textureView, const char* debugName, Handle<Texture>& outTexture);
	};

	inline Scene* loadScene(const char* path)
	{
		return Scene::load(path);
	}

	inline void destroyScene(Scene* scene)
	{
		scene->destroy();
		delete scene;
	}
}
