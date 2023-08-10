#include "stdafx.h"
#include "scene.h"
#include "core/paths.h"
#include "gfx/rhi/device.h"
#include "gfx/shaderinterop.h"

#pragma warning(push)
#pragma warning(disable: 4996) // disable _CRT_SECURE_NO_WARNINGS
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#pragma warning(pop)

#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "rhi/resourcemanager.h"

namespace limbo::Gfx
{
	namespace
	{
		struct PrimitiveData
		{
			std::vector<float3> positionStream;
			std::vector<float3> normalsStream;
			std::vector<float2> texcoordsStream;
			std::vector<uint32> indicesStream;
		};
	}

	Scene::Scene(const char* path)
	{
		LB_LOG("Starting loading %s", path);
		Core::Timer timer;

		cgltf_options options = {};
		cgltf_data* data = nullptr;
		cgltf_result result = cgltf_parse_file(&options, path, &data);
		FAILIF(result != cgltf_result_success);
		result = cgltf_load_buffers(&options, data, path);
		FAILIF(result != cgltf_result_success);

		Paths::GetPath(path, m_FolderPath);
		Paths::GetFilename(path, m_SceneName);

		// process materials
		for (size_t i = 0; i < data->materials_count; ++i)
			ProcessMaterial(&data->materials[i]);

		cgltf_scene* scene = data->scene;
		for (size_t i = 0; i < scene->nodes_count; ++i)
			ProcessNode(scene->nodes[i]);

		cgltf_free(data);

		LB_LOG("Finished loading %s (took %.2fs)", path, timer.ElapsedSeconds());
	}

	Scene* Scene::Load(const char* path)
	{
		return new Scene(path);
	}

	void Scene::Destroy()
	{
		for (const Mesh& mesh : m_Meshes)
		{
			Gfx::DestroyBuffer(mesh.VertexBuffer);
			Gfx::DestroyBuffer(mesh.IndexBuffer);
		}

		for (const auto& [name, material] : m_MeshMaterials)
		{
			Gfx::DestroyTexture(material.Albedo);
			Gfx::DestroyTexture(material.Normal);
			Gfx::DestroyTexture(material.RoughnessMetal);
			Gfx::DestroyTexture(material.Emissive);
			Gfx::DestroyTexture(material.AmbientOcclusion);
			Gfx::DestroyBuffer(material.Factors);
		}

	}

	void Scene::DrawMesh(const std::function<void(const Mesh& mesh)>& drawFunction)
	{
		for (const Mesh& m : m_Meshes)
			drawFunction(m);
	}

	MeshMaterial Scene::GetMaterial(uintptr_t pMaterial)
	{
		if (m_MeshMaterials.contains(pMaterial))
			return m_MeshMaterials[pMaterial];
		return MeshMaterial();
	}

	void Scene::ProcessNode(const cgltf_node* node)
	{
		const cgltf_mesh* mesh = node->mesh;
		if (mesh)
		{
			for (size_t i = 0; i < mesh->primitives_count; i++)
			{
				const cgltf_primitive& primitive = mesh->primitives[i];
				Mesh& m = m_Meshes.emplace_back(ProcessMesh(mesh, &primitive));
				cgltf_node_transform_world(node, &m.Transform[0][0]);
			}
		}

		// then do the same for each of its children
		for (size_t i = 0; i < node->children_count; i++)
			ProcessNode(node->children[i]);
	}

	void Scene::ProcessMaterial(const cgltf_material* material)
	{
		MaterialFactors factors = {};
		MeshMaterial& meshMaterial = m_MeshMaterials[(uintptr_t)material];
		if (material->has_pbr_metallic_roughness)
		{
			const cgltf_pbr_metallic_roughness& workflow = material->pbr_metallic_roughness;
			{
				std::string debugName = std::format(" Material({}) {}", m_MeshMaterials.size(), "Albedo");
				LoadTexture(&workflow.base_color_texture, debugName.c_str(), meshMaterial.Albedo, Format::RGBA8_UNORM_SRGB);
				factors.AlbedoFactor = glm::make_vec4(workflow.base_color_factor);
			}
			{
				std::string debugName = std::format(" Material({}) {}", m_MeshMaterials.size(), "MetallicRoughness");
				LoadTexture(&workflow.metallic_roughness_texture, debugName.c_str(), meshMaterial.RoughnessMetal, Format::RGBA8_UNORM);
				factors.RoughnessFactor = workflow.roughness_factor;
				factors.MetallicFactor  = workflow.metallic_factor;
			}
		}
		else
		{
			ensure(false);
		}

		{
			std::string debugName = std::format(" Material({}) {}", m_MeshMaterials.size(), "Normal");
			LoadTexture(&material->normal_texture, debugName.c_str(), meshMaterial.Normal, Format::RGBA8_UNORM);
		}

		{
			std::string debugName = std::format(" Material({}) {}", m_MeshMaterials.size(), "Emissive");
			LoadTexture(&material->emissive_texture, debugName.c_str(), meshMaterial.Emissive, Format::RGBA8_UNORM);
		}

		{
			std::string debugName = std::format(" Material({}) {}", m_MeshMaterials.size(), "AmbientOcclusion");
			LoadTexture(&material->occlusion_texture, debugName.c_str(), meshMaterial.AmbientOcclusion, Format::RGBA8_UNORM);
		}

		// Create factors buffer
		{
			std::string debugName = std::format(" Material({}) Factors CB", m_MeshMaterials.size());
			meshMaterial.Factors = CreateBuffer({
				.DebugName = debugName.c_str(),
				.ByteSize = sizeof(MaterialFactors),
				.Usage = BufferUsage::Constant,
				.InitialData = &factors
			});
		}
	}

	Mesh Scene::ProcessMesh(const cgltf_mesh* mesh, const cgltf_primitive* primitive)
	{
		std::string meshName;
		if (mesh->name)
			meshName = mesh->name;
		else
			meshName = m_SceneName;

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
			readAttributeData("NORMAL", primitiveData.normalsStream, 3);
			readAttributeData("TEXCOORD_0", primitiveData.texcoordsStream, 2);
		}

		// turn the attribute streams into vertex data
		size_t vertexCount = primitiveData.positionStream.size();

		std::vector<MeshVertex> vertices;
		vertices.resize(vertexCount);
		for (size_t attrIdx = 0; attrIdx < vertexCount; ++attrIdx)
		{
			MeshVertex& vertex = vertices[attrIdx];

			if (attrIdx < primitiveData.positionStream.size())
				vertex.Position = primitiveData.positionStream[attrIdx];
			if (attrIdx < primitiveData.normalsStream.size())
				vertex.Normal	= primitiveData.normalsStream[attrIdx];
			if (attrIdx < primitiveData.texcoordsStream.size())
				vertex.UV = primitiveData.texcoordsStream[attrIdx];
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

	void Scene::LoadTexture(const cgltf_texture_view* textureView, const char* debugName, Handle<Texture>& outTexture, Format format)
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
			std::string filename = std::string(m_FolderPath) + std::string(image->uri);
			data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
		}
		else
		{
			dname = m_SceneName;
			cgltf_buffer_view* bufferView = image->buffer_view;
			cgltf_buffer* buffer = bufferView->buffer;
			uint32 size = (uint32)bufferView->size;
			void* bufferLocation = (uint8*)buffer->data + bufferView->offset;
			data = stbi_load_from_memory((stbi_uc*)bufferLocation, size, &width, &height, &channels, 4);
		}
		ensure(data);

		dname += debugName;

		outTexture = CreateTexture({
			.Width = (uint32)width,
			.Height = (uint32)height,
			.MipLevels = CalculateMipCount(width),
			.DebugName = dname.c_str(),
			.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
			.Format = format,
			.Type = TextureType::Texture2D,
			.InitialData = data
		});
		free(data);

		Gfx::GenerateMipLevels(outTexture);
	}

	Mesh::Mesh(const char* meshName, const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices, uintptr_t material)
		: Transform(float4x4(1.0f))
	{
		IndexCount = indices.size();
		VertexCount = vertices.size();
		MaterialID = material;
		Name = meshName;

		// create vertex buffer
		std::string debugName = std::string(Name) + " VB";
		VertexBuffer = CreateBuffer({
			.DebugName = debugName.c_str(),
			.ByteStride = sizeof(MeshVertex),
			.ByteSize = sizeof(MeshVertex) * (uint32)vertices.size(),
			.Usage = BufferUsage::Vertex,
			.InitialData = vertices.data()
		});

		// create index buffer
		debugName = std::string(Name) + " IB";
		IndexBuffer = CreateBuffer({
			.DebugName = debugName.c_str(),
			.ByteSize = sizeof(uint32) * (uint32)indices.size(),
			.Usage = BufferUsage::Index,
			.InitialData = indices.data()
		});
	}

}
