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

		std::vector<PrimitiveData> PrimitivesStreams;
	}

	Scene::Scene(const char* path)
	{
		PrimitivesStreams.clear();

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

		// Create the geometry buffer and create the Vertex/Index buffer views for the respective meshes
		ProcessPrimitivesData();

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
			if (mesh.BLAS.IsValid()) // Some meshes don't have a BLAS set
				DestroyBuffer(mesh.BLAS);
		}

		for (Handle<Texture> texture : m_Textures)
			DestroyTexture(texture);

		DestroyBuffer(m_GeometryBuffer);
	}

	void Scene::IterateMeshes(const std::function<void(const Mesh& mesh)>& drawFunction) const
	{
		for (const Mesh& m : m_Meshes)
			drawFunction(m);
	}

	void Scene::IterateMeshesNoConst(const std::function<void(Mesh& mesh)>& drawFunction)
	{
		for (Mesh& m : m_Meshes)
			drawFunction(m);
	}

	void Scene::ProcessNode(const cgltf_node* node)
	{
		const cgltf_mesh* mesh = node->mesh;
		if (mesh)
		{
			for (size_t i = 0; i < mesh->primitives_count; i++)
			{
				const cgltf_primitive& primitive = mesh->primitives[i];
				ProcessMesh(node, mesh, &primitive);
			}
		}

		// then do the same for each of its children
		for (size_t i = 0; i < node->children_count; i++)
			ProcessNode(node->children[i]);
	}

	void Scene::ProcessMaterial(cgltf_material* cgltfMaterial)
	{
		uint32 index = (uint32)Materials.size();
		Material& material = Materials.emplace_back();
		m_MaterialPtrToIndex[cgltfMaterial] = index;
		if (cgltfMaterial->has_pbr_metallic_roughness)
		{
			const cgltf_pbr_metallic_roughness& workflow = cgltfMaterial->pbr_metallic_roughness;
			{
				std::string debugName = std::format(" Material({}) {}", index, "Albedo");
				material.AlbedoIndex  = LoadTexture(&workflow.base_color_texture, debugName.c_str(), Format::RGBA8_UNORM_SRGB);
				material.AlbedoFactor = glm::make_vec4(workflow.base_color_factor);
			}
			{
				std::string debugName		 = std::format(" Material({}) {}", index, "MetallicRoughness");
				material.RoughnessMetalIndex = LoadTexture(&workflow.metallic_roughness_texture, debugName.c_str(), Format::RGBA8_UNORM);
				material.RoughnessFactor	 = workflow.roughness_factor;
				material.MetallicFactor		 = workflow.metallic_factor;
			}
		}
		else
		{
			ensure(false);
		}

		{
			std::string debugName = std::format(" Material({}) {}", index, "Normal");
			material.NormalIndex  = LoadTexture(&cgltfMaterial->normal_texture, debugName.c_str(), Format::RGBA8_UNORM);
		}

		{
			std::string debugName   = std::format(" Material({}) {}", index, "Emissive");
			material.EmissiveIndex  = LoadTexture(&cgltfMaterial->emissive_texture, debugName.c_str(), Format::RGBA8_UNORM);
			material.EmissiveFactor = glm::make_vec3(cgltfMaterial->emissive_factor);
		}

		{
			std::string debugName			= std::format(" Material({}) {}", index, "AmbientOcclusion");
			material.AmbientOcclusionIndex  = LoadTexture(&cgltfMaterial->occlusion_texture, debugName.c_str(), Format::RGBA8_UNORM);
		}
	}

	void Scene::ProcessMesh(const cgltf_node* node, const cgltf_mesh* mesh, const cgltf_primitive* primitive)
	{
		std::string meshName;
		if (mesh->name)
			meshName = mesh->name;
		else
			meshName = m_SceneName;

		PrimitiveData& primitiveData = PrimitivesStreams.emplace_back();

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

		// process indices
		cgltf_accessor* indices = primitive->indices;
		primitiveData.indicesStream.resize(indices->count);
		for (size_t idx = 0; idx < indices->count; ++idx)
			primitiveData.indicesStream[idx] = (uint32)cgltf_accessor_read_index(primitive->indices, idx);

		cgltf_material* material = primitive->material;
		Mesh& result = m_Meshes.emplace_back();
		cgltf_node_transform_world(node, &result.Transform[0][0]);
		result.LocalMaterialIndex = m_MaterialPtrToIndex[material];
		result.Name = meshName.c_str();
	}

	void Scene::ProcessPrimitivesData()
	{
		uint64 bufferSize = 0;
		uint32 elementCount = 0;

		// Calculate the geometry buffer size
		for (size_t i = 0; i < PrimitivesStreams.size(); ++i)
		{
			bufferSize += PrimitivesStreams[i].positionStream.size()  * sizeof(float3);
			bufferSize += PrimitivesStreams[i].normalsStream.size()   * sizeof(float3);
			bufferSize += PrimitivesStreams[i].texcoordsStream.size() * sizeof(float2);
			bufferSize += PrimitivesStreams[i].indicesStream.size()   * sizeof(uint32);
		}

		elementCount = (uint32)bufferSize / sizeof(uint32);

		std::string debugName = std::format("{}_GeometryBuffer", m_SceneName);
		m_GeometryBuffer = CreateBuffer({
			.DebugName = debugName.c_str(),
			.NumElements = elementCount,
			.ByteSize = bufferSize,
			.Flags = BufferUsage::Byte | BufferUsage::ShaderResourceView
		});
		D3D12_GPU_VIRTUAL_ADDRESS geoBufferAddress = GetBuffer(m_GeometryBuffer)->Resource->GetGPUVirtualAddress();

		Handle<Buffer> upload = CreateBuffer({
			.DebugName = debugName.c_str(),
			.ByteSize = bufferSize,
			.Flags = BufferUsage::Upload
		});

		auto CopyData = [](VertexBufferView& view, uint8* data, uint64 gpuAddress, uint64& offset, const auto& stream)
		{
			auto streamSize = stream.size() * sizeof(stream[0]);
			view = {
				.BufferLocation = gpuAddress + offset,
				.SizeInBytes = (uint32)streamSize,
				.StrideInBytes = sizeof(stream[0]),
				.Offset = (uint32)offset
			};
			memcpy(data + offset, stream.data(), streamSize);
			offset += streamSize;
		};

		Map(upload);
		uint8* data = (uint8*)GetMappedData(upload);
		uint64 dataOffset = 0;
		for (size_t i = 0; i < PrimitivesStreams.size(); ++i)
		{
			check(dataOffset % sizeof(uint32) == 0); // the offset is a 32bit value, do not let it overflow

			CopyData(m_Meshes[i].PositionsLocation, data, geoBufferAddress, dataOffset, PrimitivesStreams[i].positionStream);
			CopyData(m_Meshes[i].NormalsLocation, data, geoBufferAddress, dataOffset, PrimitivesStreams[i].normalsStream);
			CopyData(m_Meshes[i].TexCoordsLocation, data, geoBufferAddress, dataOffset, PrimitivesStreams[i].texcoordsStream);

			size_t streamSize = PrimitivesStreams[i].indicesStream.size() * sizeof(uint32);
			m_Meshes[i].IndicesLocation = {
				.BufferLocation = geoBufferAddress + dataOffset,
				.SizeInBytes = (uint32)streamSize,
				.Offset = (uint32)dataOffset
			};
			memcpy(data + dataOffset, PrimitivesStreams[i].indicesStream.data(), streamSize);
			dataOffset += streamSize;

			m_Meshes[i].IndexCount  = PrimitivesStreams[i].indicesStream.size();
			m_Meshes[i].VertexCount = PrimitivesStreams[i].positionStream.size();
		}

		CopyBufferToBuffer(upload, m_GeometryBuffer, bufferSize);
		Device::Ptr->GetCommandContext(ContextType::Direct)->TransitionResource(m_GeometryBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
		DestroyBuffer(upload);
	}

	int Scene::LoadTexture(const cgltf_texture_view* textureView, const char* debugName, Format format)
	{
		if (!textureView->texture)
			return -1;

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

		Handle<Texture> texture = CreateTexture({
			.Width = (uint32)width,
			.Height = (uint32)height,
			.MipLevels = CalculateMipCount(width),
			.DebugName = dname.c_str(),
			.Flags = TextureUsage::UnorderedAccess | TextureUsage::ShaderResource,
			.Format = format,
			.Type = TextureType::Texture2D,
			.InitialData = data
		});
		free(data);

		Gfx::GenerateMipLevels(texture);
		m_Textures.push_back(texture);

		Texture* t = ResourceManager::Ptr->GetTexture(texture);
		FAILIF(!t, -1);
		return t->SRVHandle.Index;
	}
}
