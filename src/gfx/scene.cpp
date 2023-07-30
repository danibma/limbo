#include "stdafx.h"
#include "scene.h"
#include "core/paths.h"

#pragma warning(push)
#pragma warning(disable: 4996) // disable _CRT_SECURE_NO_WARNINGS
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#pragma warning(pop)

#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "rhi/resourcemanager.h"

namespace limbo::gfx
{
	namespace
	{
		struct PrimitiveData
		{
			std::vector<float3> positionStream;
			std::vector<float2> texcoordsStream;
			std::vector<uint32> indicesStream;
		};
	}

	Scene::Scene(const char* path)
	{
		LB_LOG("Starting loading %s", path);
		core::Timer timer;

		cgltf_options options = {};
		cgltf_data* data = nullptr;
		cgltf_result result = cgltf_parse_file(&options, path, &data);
		FAILIF(result != cgltf_result_success);
		result = cgltf_load_buffers(&options, data, path);
		FAILIF(result != cgltf_result_success);

		paths::getPath(path, m_folderPath);
		paths::getFilename(path, m_sceneName);

		// process materials
		for (size_t i = 0; i < data->materials_count; ++i)
			processMaterial(&data->materials[i]);

		cgltf_scene* scene = data->scene;
		for (size_t i = 0; i < scene->nodes_count; ++i)
			processNode(scene->nodes[i]);

		cgltf_free(data);

		LB_LOG("Finished loading %s (took %.2fs)", path, timer.ElapsedSeconds());
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
		}

		for (const auto& [name, material] : m_meshMaterials)
		{
			gfx::destroyTexture(material.albedo);
			gfx::destroyTexture(material.roughnessMetal);
			gfx::destroyTexture(material.normal);
			gfx::destroyTexture(material.emissive);
		}
	}

	void Scene::drawMesh(const std::function<void(const Mesh& mesh)>& drawFunction)
	{
		for (const Mesh& m : m_meshes)
			drawFunction(m);
	}

	MeshMaterial Scene::getMaterial(uintptr_t pMaterial)
	{
		if (m_meshMaterials.contains(pMaterial))
			return m_meshMaterials[pMaterial];
		return MeshMaterial();
	}

	void Scene::processNode(const cgltf_node* node)
	{
		const cgltf_mesh* mesh = node->mesh;
		if (mesh)
		{
			for (size_t i = 0; i < mesh->primitives_count; i++)
			{
				const cgltf_primitive& primitive = mesh->primitives[i];
				Mesh& m = m_meshes.emplace_back(processMesh(mesh, &primitive));
				cgltf_node_transform_world(node, &m.transform[0][0]);
			}
		}

		// then do the same for each of its children
		for (size_t i = 0; i < node->children_count; i++)
			processNode(node->children[i]);
	}

	void Scene::processMaterial(const cgltf_material* material)
	{
		MeshMaterial& meshMaterial = m_meshMaterials[(uintptr_t)material];
		if (material->has_pbr_metallic_roughness)
		{
			const cgltf_pbr_metallic_roughness& workflow = material->pbr_metallic_roughness;
			{
				std::string debugName = std::format(" Material({}) {}", m_meshMaterials.size(), "Albedo");
				loadTexture(&workflow.base_color_texture, debugName.c_str(), meshMaterial.albedo);
			}
			{
				std::string debugName = std::format(" Material({}) {}", m_meshMaterials.size(), "MetallicRoughness");
				loadTexture(&workflow.metallic_roughness_texture, debugName.c_str(), meshMaterial.roughnessMetal);
			}
		}
		else
		{
			ensure(false);
		}

		{
			std::string debugName = std::format(" Material({}) {}", m_meshMaterials.size(), "Emissive");
			loadTexture(&material->emissive_texture, debugName.c_str(), meshMaterial.emissive);
		}
	}

	Mesh Scene::processMesh(const cgltf_mesh* mesh, const cgltf_primitive* primitive)
	{
		std::string meshName;
		if (mesh->name)
			meshName = mesh->name;
		else
			meshName = m_sceneName;

		PrimitiveData primitiveData;

		// process vertices
		for (size_t attributeIndex = 0; attributeIndex < primitive->attributes_count; ++attributeIndex)
		{
			const cgltf_attribute& attribute = primitive->attributes[attributeIndex];

			auto readAttributeData = [&attribute](const char* name, auto& stream, uint32 numComponents)
			{
				if (strcmp(attribute.name, name) == 0)
				{
					stream.resize(attribute.data->count);
					for (size_t i = 0; i < attribute.data->count; ++i)
					{
						ensure(cgltf_accessor_read_float(attribute.data, i, &stream[i].x, numComponents));
					}
				}
			};

			readAttributeData("POSITION", primitiveData.positionStream, 3);
			readAttributeData("TEXCOORD_0", primitiveData.texcoordsStream, 2);
		}

		// turn the attribute streams into vertex data
		size_t vertexCount = primitiveData.positionStream.size();
		ensure(primitiveData.texcoordsStream.size() == vertexCount);

		std::vector<MeshVertex> vertices;
		vertices.resize(vertexCount);
		for (size_t attrIdx = 0; attrIdx < vertexCount; ++attrIdx)
		{
			MeshVertex& vertex = vertices[attrIdx];
			vertex.position = primitiveData.positionStream[attrIdx];
			vertex.uv		= primitiveData.texcoordsStream[attrIdx];
		}

		// process indices
		cgltf_accessor* indices = primitive->indices;
		primitiveData.indicesStream.resize(indices->count);
		for (size_t idx = 0; idx < indices->count; ++idx)
			primitiveData.indicesStream[idx] = (uint32)cgltf_accessor_read_index(primitive->indices, idx);

		cgltf_material* material = primitive->material;
		uintptr_t materialID = (uintptr_t)material;

		return Mesh(meshName.c_str(), vertices, primitiveData.indicesStream, materialID);
	}

	void Scene::loadTexture(const cgltf_texture_view* textureView, const char* debugName, Handle<Texture>& outTexture)
	{
		if (!textureView->texture)
			return;

		int width	 = 0;
		int height	 = 0;
		int channels = 0;
		void* data;

		std::string dname;
		cgltf_image* image = textureView->texture->image;
		if (image->uri)
		{
			dname = image->uri;
			std::string filename = std::string(m_folderPath) + std::string(image->uri);
			data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
		}
		else
		{
			dname = m_sceneName;
			cgltf_buffer_view* bufferView = image->buffer_view;
			cgltf_buffer* buffer = bufferView->buffer;
			uint32 size = (uint32)bufferView->size;
			void* bufferLocation = (uint8*)buffer->data + bufferView->offset;
			data = stbi_load_from_memory((stbi_uc*)bufferLocation, size, &width, &height, &channels, 4);
		}
		ensure(data);

		dname += debugName;

		outTexture = createTexture({
			.width = (uint32)width,
			.height = (uint32)height,
			.debugName = dname.c_str(),
			.format = Format::RGBA8_UNORM,
			.type = TextureType::Texture2D,
			.initialData = data
		});
		free(data);
	}

	Mesh::Mesh(const char* meshName, const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, uintptr_t material)
		: transform(float4x4(1.0f))
	{
		indexCount = indices.size();
		vertexCount = vertices.size();
		materialID = material;
		name = meshName;

		// create vertex buffer
		std::string debugName = std::string(name) + " VB";
		vertexBuffer = createBuffer({
			.debugName = debugName.c_str(),
			.byteStride = sizeof(MeshVertex),
			.byteSize = sizeof(MeshVertex) * (uint32)vertices.size(),
			.usage = BufferUsage::Vertex,
			.initialData = vertices.data()
		});

		// create index buffer
		debugName = std::string(name) + " IB";
		indexBuffer = createBuffer({
			.debugName = debugName.c_str(),
			.byteSize = sizeof(uint32) * (uint32)indices.size(),
			.usage = BufferUsage::Index,
			.initialData = indices.data()
		});
	}

}
