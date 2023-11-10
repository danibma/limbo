#include "stdafx.h"
#include "scene.h"
#include "core/paths.h"
#include "gfx/rhi/commandcontext.h"
#include "gfx/shaderinterop.h"
#include "core/jobsystem.h"
#include "rhi/resourcemanager.h"
#include "core/timer.h"

#pragma warning(push)
#pragma warning(disable: 4996) // disable _CRT_SECURE_NO_WARNINGS
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#pragma warning(pop)

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <meshoptimizer.h>

namespace limbo::Gfx
{
	namespace
	{
		struct PrimitiveData
		{
			std::vector<float3> PositionStream;
			std::vector<float3> NormalsStream;
			std::vector<float2> TexCoordsStream;
			std::vector<uint32> IndicesStream;
		};
		std::vector<PrimitiveData> PrimitivesStreams;

		struct TextureData
		{
			std::string Name;
			int			Width = 0;
			int			Height = 0;
			int			Channels = 0;
			void*		Data;
		};

		std::vector<TextureData> TextureStreams;
		// map the cgltf_texture to the index in TextureStreams
		std::unordered_map<uintptr_t, uint32> TexturesMap;

		void OptimizePrimitiveData(PrimitiveData& primitiveData)
		{
			size_t indexCount = primitiveData.IndicesStream.size();
			size_t vertexCount = primitiveData.PositionStream.size();

			std::vector<uint32> remap(vertexCount);
			meshopt_optimizeVertexFetchRemap(&remap[0], primitiveData.IndicesStream.data(), indexCount, vertexCount);

			meshopt_remapIndexBuffer(primitiveData.IndicesStream.data(), primitiveData.IndicesStream.data(), indexCount, remap.data());
			meshopt_remapVertexBuffer(primitiveData.PositionStream.data(), primitiveData.PositionStream.data(), vertexCount, sizeof(float3), remap.data());
			meshopt_remapVertexBuffer(primitiveData.NormalsStream.data(), primitiveData.NormalsStream.data(), vertexCount, sizeof(float3), remap.data());
			if (primitiveData.TexCoordsStream.size() > 0)
				meshopt_remapVertexBuffer(primitiveData.TexCoordsStream.data(), primitiveData.TexCoordsStream.data(), vertexCount, sizeof(float2), remap.data());

			meshopt_optimizeVertexCache(primitiveData.IndicesStream.data(), primitiveData.IndicesStream.data(), indexCount, vertexCount);
			meshopt_optimizeOverdraw(primitiveData.IndicesStream.data(), primitiveData.IndicesStream.data(), indexCount, &primitiveData.PositionStream[0].x, vertexCount, sizeof(float3), 1.05f);
		}
	}

	Scene::Scene(const char* path)
	{
		LB_LOG("Starting loading %s", path);
		Core::Timer timer;

		cgltf_options options = {};
		cgltf_data* data = nullptr;
		cgltf_result result = cgltf_parse_file(&options, path, &data);
		ENSURE_RETURN(result != cgltf_result_success);
		result = cgltf_load_buffers(&options, data, path);
		ENSURE_RETURN(result != cgltf_result_success);

		Paths::GetPath(path, m_FolderPath);
		Paths::GetFilename(path, m_SceneName);

		// load all textures
#if 0
		for (size_t i = 0; i < data->textures_count; ++i)
			LoadTexture(&data->textures[i]);
#else
		Core::JobSystem::ExecuteMany((uint32)data->textures_count, Math::Max((uint32)data->textures_count / Core::JobSystem::ThreadCount(), 1u), Core::TOnJobSystemExecuteMany::CreateLambda([this, data](Core::JobDispatchArgs args)
		{
			LoadTexture(&data->textures[args.jobIndex]);
		}));
		Core::JobSystem::WaitIdle();
#endif

		// process materials
		for (size_t i = 0; i < data->materials_count; ++i)
			ProcessMaterial(&data->materials[i]);

		cgltf_scene* scene = data->scene;
		for (size_t i = 0; i < scene->nodes_count; ++i)
			ProcessNode(scene->nodes[i]);

		cgltf_free(data);

		// Create the geometry buffer and create the Vertex/Index buffer views for the respective meshes
		ProcessPrimitivesData();

		LB_LOG("Finished loading %s (took %.3fs)", path, timer.ElapsedSeconds());

		// Clear streams
		PrimitivesStreams.clear();
		TexturesMap.clear();
		for (TextureData& texture : TextureStreams)
		{
			free(texture.Data);
			texture.Data = nullptr;
		}
		TextureStreams.clear();
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

		for (RHI::TextureHandle texture : m_Textures)
			RHI::DestroyTexture(texture);

		DestroyBuffer(m_GeometryBuffer);
	}

	void Scene::IterateMeshes(TOnDrawMesh drawDelegate) const
	{
		for (const Mesh& m : m_Meshes)
			drawDelegate.ExecuteIfBound(m);
	}

	void Scene::IterateMeshesNoConst(TOnDrawMeshNoConst drawDelegate)
	{
		for (Mesh& m : m_Meshes)
			drawDelegate.ExecuteIfBound(m);
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
				material.AlbedoIndex  = CreateTextureResource(&workflow.base_color_texture, debugName.c_str(), RHI::Format::RGBA8_UNORM_SRGB);
				material.AlbedoFactor = float4(workflow.base_color_factor[0], workflow.base_color_factor[1], workflow.base_color_factor[2], workflow.base_color_factor[3]);
			}
			{
				std::string debugName = std::format(" Material({}) {}", index, "MetallicRoughness");
				material.RoughnessMetalIndex = CreateTextureResource(&workflow.metallic_roughness_texture, debugName.c_str(), RHI::Format::RGBA8_UNORM);
				material.RoughnessFactor = workflow.roughness_factor;
				material.MetallicFactor = workflow.metallic_factor;
			}
		}
		else
		{
			ensure(false);
		}

		{
			std::string debugName = std::format(" Material({}) {}", index, "Normal");
			material.NormalIndex = CreateTextureResource(&cgltfMaterial->normal_texture, debugName.c_str(), RHI::Format::RGBA8_UNORM);
		}

		{
			std::string debugName   = std::format(" Material({}) {}", index, "Emissive");
			material.EmissiveIndex  = CreateTextureResource(&cgltfMaterial->emissive_texture, debugName.c_str(), RHI::Format::RGBA8_UNORM_SRGB);
			material.EmissiveFactor = float3(cgltfMaterial->emissive_factor[0], cgltfMaterial->emissive_factor[1], cgltfMaterial->emissive_factor[2]);
		}

		{
			std::string debugName			= std::format(" Material({}) {}", index, "AmbientOcclusion");
			material.AmbientOcclusionIndex  = CreateTextureResource(&cgltfMaterial->occlusion_texture, debugName.c_str(), RHI::Format::RGBA8_UNORM);
		}
	}


	void Scene::LoadTexture(const cgltf_texture* texture)
	{
		if (!texture)
			return;

		int width = 0;
		int height = 0;
		int channels = 0;
		void* data;

		std::string dname;
		cgltf_image* image = texture->image;
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


		std::scoped_lock<std::mutex> lock(m_AddToTextureMapMutex);
		TexturesMap[(uintptr_t)texture] = (uint32)TextureStreams.size();
		TextureStreams.emplace_back(dname, width, height, channels, data);
	}

	uint Scene::CreateTextureResource(const cgltf_texture_view* textureView, const std::string& debugName, RHI::Format format)
	{
		if (!textureView->texture)
			return -1;

		TextureData& textureData = TextureStreams.at(TexturesMap[(uintptr_t)textureView->texture]);
		check(textureData.Data);
		textureData.Name += debugName;

		RHI::TextureHandle texture = RHI::CreateTexture({
			.Width = (uint32)textureData.Width,
			.Height = (uint32)textureData.Height,
			.MipLevels = RHI::CalculateMipCount(textureData.Width),
			.DebugName = textureData.Name.c_str(),
			.Flags = RHI::TextureUsage::UnorderedAccess | RHI::TextureUsage::ShaderResource,
			.Format = format,
			.Type = RHI::TextureType::Texture2D,
			.InitialData = textureData.Data
		});

		RHI::CommandContext::GetCommandContext()->GenerateMipLevels(texture);
		m_Textures.push_back(texture);

		RHI::Texture* t = RM_GET(texture);
		ENSURE_RETURN(!t, -1);
		return t->SRV();
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

			readAttributeData("POSITION", primitiveData.PositionStream, 3);
			readAttributeData("NORMAL", primitiveData.NormalsStream, 3);
			readAttributeData("TEXCOORD_0", primitiveData.TexCoordsStream, 2);
		}

		// process indices
		cgltf_accessor* indices = primitive->indices;
		primitiveData.IndicesStream.resize(indices->count);
		for (size_t idx = 0; idx < indices->count; ++idx)
			primitiveData.IndicesStream[idx] = (uint32)cgltf_accessor_read_index(primitive->indices, idx);

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
			bufferSize += PrimitivesStreams[i].PositionStream.size()  * sizeof(float3);
			bufferSize += PrimitivesStreams[i].NormalsStream.size()   * sizeof(float3);
			bufferSize += PrimitivesStreams[i].TexCoordsStream.size() * sizeof(float2);
			bufferSize += PrimitivesStreams[i].IndicesStream.size()   * sizeof(uint32);
		}

		elementCount = (uint32)bufferSize / sizeof(uint32);

		std::string debugName = std::format("{}_GeometryBuffer", m_SceneName);
		m_GeometryBuffer = RHI::CreateBuffer({
			.DebugName = debugName.c_str(),
			.NumElements = elementCount,
			.ByteSize = bufferSize,
			.Flags = RHI::BufferUsage::Byte | RHI::BufferUsage::ShaderResourceView
		});
		D3D12_GPU_VIRTUAL_ADDRESS geoBufferAddress = RM_GET(m_GeometryBuffer)->Resource->GetGPUVirtualAddress();

		RHI::BufferHandle upload = RHI::CreateBuffer({
			.DebugName = debugName.c_str(),
			.ByteSize = bufferSize,
			.Flags = RHI::BufferUsage::Upload
		});

		auto CopyData = [](RHI::VertexBufferView& view, uint8* data, uint64 gpuAddress, uint64& offset, const auto& stream)
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

		RHI::Buffer* pUploadBuffer = RM_GET(upload);
		pUploadBuffer->Map();
		uint8* data = (uint8*)pUploadBuffer->MappedData;
		uint64 dataOffset = 0;
		for (size_t i = 0; i < PrimitivesStreams.size(); ++i)
		{
			check(dataOffset % sizeof(uint32) == 0); // the offset is a 32bit value, do not let it overflow

			OptimizePrimitiveData(PrimitivesStreams[i]);

			CopyData(m_Meshes[i].PositionsLocation, data, geoBufferAddress, dataOffset, PrimitivesStreams[i].PositionStream);
			CopyData(m_Meshes[i].NormalsLocation, data, geoBufferAddress, dataOffset, PrimitivesStreams[i].NormalsStream);
			CopyData(m_Meshes[i].TexCoordsLocation, data, geoBufferAddress, dataOffset, PrimitivesStreams[i].TexCoordsStream);

			size_t streamSize = PrimitivesStreams[i].IndicesStream.size() * sizeof(uint32);
			m_Meshes[i].IndicesLocation = {
				.BufferLocation = geoBufferAddress + dataOffset,
				.SizeInBytes = (uint32)streamSize,
				.Offset = (uint32)dataOffset
			};
			memcpy(data + dataOffset, PrimitivesStreams[i].IndicesStream.data(), streamSize);
			dataOffset += streamSize;

			m_Meshes[i].IndexCount  = PrimitivesStreams[i].IndicesStream.size();
			m_Meshes[i].VertexCount = PrimitivesStreams[i].PositionStream.size();
		}

		RHI::CommandContext::GetCommandContext()->CopyBufferToBuffer(upload, m_GeometryBuffer, bufferSize);
		RHI::CommandContext::GetCommandContext()->InsertResourceBarrier(m_GeometryBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		DestroyBuffer(upload);
	}
}
