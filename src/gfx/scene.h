#pragma once

#include <functional>

#include "core/math.h"

#include "rhi/resourcepool.h"

struct aiNode;
struct aiScene;
struct aiMaterial;
struct aiMesh;
enum   aiTextureType;

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
		Handle<Texture> diffuse;
		Handle<Texture> roughness;
	};

	struct Mesh
	{
		Handle<Buffer>			vertexBuffer;
		Handle<Buffer>			indexBuffer;

		MeshMaterial			material;

		float4x4				transform;

		size_t					indexCount = 0;
		size_t					vertexCount = 0;

		const char*				name;

		Mesh(const char* meshName, const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, const MeshMaterial& meshMaterial);
	};

	class Scene
	{
		std::vector<Mesh>		m_meshes;
		char					m_folderPath[256];

	protected:
		Scene() = default;
		Scene(const char* path);

	public:
		static Scene* load(const char* path);
		void destroy();

		void drawMesh(const std::function<void(const Mesh& mesh)>& drawFunction);

	private:
		void processNode(aiNode* node, const aiScene* scene);
		Mesh processMesh(aiMesh* mesh, const aiScene* scene);

		void* loadTexture(const aiMaterial* material, const aiScene* scene, aiTextureType type, int* width, int* height, int* channels);
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
