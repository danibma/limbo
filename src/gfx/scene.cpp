#include "scene.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "rhi/resourcemanager.h"

namespace limbo::gfx
{
	Scene::Scene(const char* path)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			LB_ERROR("Could not load '%s' model: %s", path, importer.GetErrorString());

		std::filesystem::path temp = path;
		m_folderPath = temp.remove_filename().string().c_str();

		processNode(scene->mRootNode, scene);
	}

	Scene* Scene::load(const char* path)
	{
		return new Scene(path);
	}

	void Scene::destroy()
	{
		for (const Mesh& mesh : m_meshes)
		{
			gfx::destroyBuffer(mesh.vertexBuffer);
			gfx::destroyBuffer(mesh.indexBuffer);
			gfx::destroyTexture(mesh.material.diffuse);
			gfx::destroyTexture(mesh.material.roughness);
		}
	}

	void Scene::drawMesh(const std::function<void(const Mesh& mesh)>& drawFunction)
	{
		for (const Mesh& m : m_meshes)
			drawFunction(m);
	}

	void Scene::processNode(aiNode* node, const aiScene* scene)
	{
		// process all the node's meshes (if any)
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			Mesh& m = m_meshes.emplace_back(processMesh(mesh, scene));
			m.transform = aiMatrix4x4ToGlm(&node->mTransformation);
			aiNode* parent = node->mParent;
			while (parent)
			{
				m.transform *= aiMatrix4x4ToGlm(&parent->mTransformation);
				parent = parent->mParent;
			}
		}

		// then do the same for each of its children
		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh Scene::processMesh(aiMesh* mesh, const aiScene* scene)
	{
		std::vector<MeshVertex> vertices;
		vertices.reserve(mesh->mNumVertices);
		std::vector<uint32_t> indices;
		indices.reserve(mesh->mNumFaces * 3);

		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			MeshVertex vertex;

			glm::vec3 vector;
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.position = vector;

			if (mesh->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.uv = vec;
			}
			else
			{
				vertex.uv = { 0, 0 };
			}

			vertices.emplace_back(vertex);
		}

		// process indices
		for (uint32_t i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (uint32_t j = 0; j < face.mNumIndices; j++)
				indices.emplace_back(face.mIndices[j]);
		}

		// process textures
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		MeshMaterial modelMaterial;
		// Get diffuse texture
		{
			int width, height, channels;
			void* data = loadTexture(material, scene, aiTextureType_DIFFUSE, &width, &height, &channels);

			std::string debugName = std::string(mesh->mName.C_Str()) + " Diffuse";
			modelMaterial.diffuse = ResourceManager::ptr->createTexture({
				.width = (uint32)width,
				.height = (uint32)height,
				.debugName = debugName.c_str(),
				.format = Format::RGBA8_UNORM,
				.type = TextureType::Texture2D,
				.initialData = data
			});
			free(data);
		}

		// Get roughness texture
		{
			int width, height, channels;
			void* data = loadTexture(material, scene, aiTextureType_SHININESS, &width, &height, &channels);

			std::string debugName = std::string(mesh->mName.C_Str()) + " Roughness";
			modelMaterial.roughness = ResourceManager::ptr->createTexture({
				.width = (uint32)width,
				.height = (uint32)height,
				.debugName = debugName.c_str(),
				.format = Format::RGBA8_UNORM,
				.type = TextureType::Texture2D,
				.initialData = data
			});
			free(data);
		}

		return Mesh(mesh->mName.C_Str(), vertices, indices, modelMaterial);
	}

	void* Scene::loadTexture(const aiMaterial* material, const aiScene* scene, aiTextureType type, int* width, int* height, int* channels)
	{
		*width = 1;
		*height = 1;
		*channels = 4;

		int count = material->GetTextureCount(type);
		ensure(count <= 1); // if this breaks here, it's time to update this _syste
		if (count == 1)
		{
			aiString path;
			aiReturn result = material->GetTexture(type, 0, &path);
			ensure(result == aiReturn_SUCCESS);
			// check if the texture is embedded or if we need to load it from a file
			if (const aiTexture* texture = scene->GetEmbeddedTexture(path.C_Str()))
			{
				ensure(texture->mHeight == 0); // read mWidth comment

				unsigned char* data = stbi_load_from_memory((stbi_uc*)texture->pcData, texture->mWidth, width, height, channels, 4);
				ensure(data);
				return data;
			}
			else
			{
				std::string filename = std::string(m_folderPath) + std::string(path.C_Str());
				unsigned char* data = stbi_load(filename.c_str(), width, height, channels, STBI_rgb_alpha); // Load the image as RGBA
				if (!data)
					LB_ERROR("Failed to load texture file '%s'", path.C_Str());
				return data;
			}
		}

		aiColor3D color(0.f, 0.f, 0.f);
		material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
		glm::vec3* baseColor = new glm::vec3(color.r, color.g, color.b);

		return baseColor;
	}

	Mesh::Mesh(const char* meshName, const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, const MeshMaterial& meshMaterial)
	{
		indexCount = indices.size();
		vertexCount = vertices.size();
		material = meshMaterial;
		name = meshName;

		// create vertex buffer
		std::string debugName = std::string(name) + " VB";
		vertexBuffer = ResourceManager::ptr->createBuffer({
			.debugName = debugName.c_str(),
			.byteStride = sizeof(MeshVertex),
			.byteSize = sizeof(MeshVertex) * (uint32)vertices.size(),
			.usage = BufferUsage::Vertex,
			.initialData = vertices.data()
		});

		// create index buffer
		debugName = std::string(name) + " IB";
		indexBuffer = ResourceManager::ptr->createBuffer({
			.debugName = debugName.c_str(),
			.byteSize = sizeof(uint32) * (uint32)indices.size(),
			.usage = BufferUsage::Index,
			.initialData = indices.data()
		});
	}

}
