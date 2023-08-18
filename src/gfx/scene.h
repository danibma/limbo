#pragma once

#include <functional> // for std::function
#include <cgltf/cgltf.h>

#include "core/math.h"
#include "gfx/shaderinterop.h"

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

	struct Mesh
	{
		VertexBufferView		PositionsLocation;
		VertexBufferView		NormalsLocation;
		VertexBufferView		TexCoordsLocation;
		IndexBufferView			IndicesLocation;

		Handle<Buffer>			BLAS;

		uint32					LocalMaterialIndex;
		uint32					InstanceID;

		float4x4				Transform;

		size_t					IndexCount = 0;
		size_t					VertexCount = 0;

		const char*				Name;
	};

	class Scene
	{
		std::vector<Mesh>								m_Meshes;
		std::vector<Handle<Texture>>					m_Textures;
		char											m_FolderPath[256];
		char											m_SceneName[128];

		std::unordered_map<cgltf_material*, uint32>		m_MaterialPtrToIndex;

		// this will contains all the geometry information about all the meshes
		Handle<Buffer>									m_GeometryBuffer;

	public:
		std::vector<Material>							Materials;

	protected:
		Scene() = default;
		Scene(const char* path);

	public:
		static Scene* Load(const char* path);
		void Destroy();

		void IterateMeshes(const std::function<void(const Mesh& mesh)>& drawFunction) const;
		void IterateMeshesNoConst(const std::function<void(Mesh& mesh)>& drawFunction);

		uint32 NumMeshes() const { return (uint32)m_Meshes.size(); }

		Buffer* GetGeometryBuffer() const { return GetBuffer(m_GeometryBuffer); }

	private:
		void ProcessNode(const cgltf_node* node);
		void ProcessMaterial(cgltf_material* cgltfMaterial);
		void ProcessMesh(const cgltf_node* node, const cgltf_mesh* mesh, const cgltf_primitive* primitive);
		void ProcessPrimitivesData();

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
