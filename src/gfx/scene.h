#pragma once

#include <cgltf/cgltf.h>

#include "core/math.h"
#include "gfx/shaderinterop.h"

#include "rhi/resourcemanager.h"

struct cgltf_node;
struct cgltf_scene;
struct cgltf_mesh;
struct cgltf_primitive;
struct cgltf_texture_view;
struct cgltf_material;

namespace limbo::RHI
{
	class Texture;
	class Buffer;
}

namespace limbo::Gfx
{
	struct Mesh
	{
		RHI::VertexBufferView		PositionsLocation;
		RHI::VertexBufferView		NormalsLocation;
		RHI::VertexBufferView		TexCoordsLocation;
		RHI::IndexBufferView		IndicesLocation;

		RHI::BufferHandle	BLAS;

		uint32						LocalMaterialIndex;
		uint32						InstanceID;

		float4x4					Transform;

		size_t						IndexCount = 0;
		size_t						VertexCount = 0;

		const char*					Name;
	};
	DECLARE_DELEGATE(TOnDrawMesh, const Mesh&);
	DECLARE_DELEGATE(TOnDrawMeshNoConst, Mesh&);

	class Scene
	{
		std::vector<Mesh>								m_Meshes;
		std::vector<RHI::TextureHandle>					m_Textures;
		char											m_FolderPath[256];
		char											m_SceneName[128];

		std::unordered_map<cgltf_material*, uint32>		m_MaterialPtrToIndex;

		// this will contains all the geometry information about all the meshes
		RHI::BufferHandle								m_GeometryBuffer;

		std::mutex										m_AddToTextureMapMutex;

	public:
		std::vector<Material>							Materials;

	protected:
		Scene() = default;
		Scene(const char* path);

	public:
		static Scene* Load(const char* path);
		void Destroy();

		void IterateMeshes(TOnDrawMesh drawDelegate) const;
		void IterateMeshesNoConst(TOnDrawMeshNoConst drawDelegate);

		uint32 NumMeshes() const { return (uint32)m_Meshes.size(); }

		RHI::Buffer* GetGeometryBuffer() const { return RM_GET(m_GeometryBuffer); }

	private:
		void ProcessNode(const cgltf_node* node);
		void ProcessMaterial(cgltf_material* cgltfMaterial);
		void ProcessMesh(const cgltf_node* node, const cgltf_mesh* mesh, const cgltf_primitive* primitive);
		void ProcessPrimitivesData();

		void LoadTexture(const cgltf_texture* texture);
		uint CreateTextureResource(const cgltf_texture_view* textureView, const std::string& debugName, RHI::Format format);
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
