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

namespace limbo::Gfx
{
	class Texture;
	class Buffer;

	struct MeshVertex
	{
		float3 Position;
		float3 Normal;
		float2 UV;
	};

	struct Mesh
	{
		Handle<Buffer>			VertexBuffer;
		Handle<Buffer>			IndexBuffer;
		Handle<Buffer>			BLAS;

		Handle<Buffer>			Material;

		float4x4				Transform;

		size_t					IndexCount = 0;
		size_t					VertexCount = 0;

		const char*				Name;

		Mesh(const char* meshName, const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, Handle<Buffer> material);
	};

	class Scene
	{
		std::vector<Mesh>								m_Meshes;
		std::vector<Handle<Texture>>					m_Textures;
		char											m_FolderPath[256];
		char											m_SceneName[128];

		std::unordered_map<uintptr_t, Handle<Buffer>>	m_MeshMaterials;

	protected:
		Scene() = default;
		Scene(const char* path);

	public:
		static Scene* Load(const char* path);
		void Destroy();

		void IterateMeshes(const std::function<void(const Mesh& mesh)>& drawFunction) const;
		void IterateMeshesNoConst(const std::function<void(Mesh& mesh)>& drawFunction);

		uint32 NumMeshes() const { return (uint32)m_Meshes.size(); }

	private:
		void ProcessNode(const cgltf_node* node);
		void ProcessMaterial(const cgltf_material* cgltfMaterial);
		Mesh ProcessMesh(const cgltf_mesh* mesh, const cgltf_primitive* primitive);

		int LoadTexture(const cgltf_texture_view* textureView, const char* debugName, Format format);
	};

	inline Scene* LoadScene(const char* path)
	{
		return Scene::Load(path);
	}

	inline void DestroyScene(Scene* scene)
	{
		scene->Destroy();
		delete scene;
	}
}
