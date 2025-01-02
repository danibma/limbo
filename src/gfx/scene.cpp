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
#include <dds/dds.h>
#include <meshoptimizer.h>

#include "core/utils.h"

namespace limbo::Gfx
{
	namespace
	{
		struct PrimitiveData
		{
			std::vector<float3>		PositionStream;
			std::vector<float3>		NormalsStream;
			std::vector<float4>		TangentsStream;
			std::vector<float2>		TexCoordsStream;

			std::vector<MeshVertex> VerticesStream;
			std::vector<uint32>		IndicesStream;

			std::vector<Meshlet>			Meshlets;
			std::vector<uint32>				MeshletVertices;
			std::vector<Meshlet::Triangle>	MeshletTriangles;
		};
		std::vector<PrimitiveData> PrimitivesStreams;

		struct TextureData
		{
			std::string Name;
			int			Width = 0;
			int			Height = 0;
			int			Channels = 0;
			void*		Data = nullptr;
			uint16		NumMips = 1;
			bool		bGenerateMips = true;
			RHI::Format Format;
		};

		std::vector<TextureData> TextureStreams;
		// map the cgltf_texture to the index in TextureStreams
		std::unordered_map<uintptr_t, uint32> TexturesMap;

		void CopyVertexData(RHI::VertexBufferView& view, uint8* data, uint64 gpuAddress, uint64& offset, const auto& stream)
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
		}

		void CopyMeshletData(uint32& currentDataOffset, uint8* data, uint64& offset, const auto& stream)
		{
			auto streamSize = stream.size() * sizeof(stream[0]);
			currentDataOffset = (uint32)offset;
			memcpy(data + offset, stream.data(), streamSize);
			offset += streamSize;
		}

		void CreateVertexStream(PrimitiveData& primitiveData)
		{
			size_t vertexCount = primitiveData.PositionStream.size();

			primitiveData.VerticesStream.resize(vertexCount);
			for (int i = 0; i < vertexCount; ++i)
			{
				MeshVertex& vertex = primitiveData.VerticesStream[i];
				vertex = {};

				vertex.Position = primitiveData.PositionStream[i];
				if (primitiveData.NormalsStream.size() > i) vertex.Normal = primitiveData.NormalsStream[i];
				if (primitiveData.TexCoordsStream.size() > i) vertex.UV = primitiveData.TexCoordsStream[i];
				if (primitiveData.TangentsStream.size() > i) vertex.Tangent = primitiveData.TangentsStream[i];
			}
		}
		
		void OptimizePrimitiveData(PrimitiveData& primitiveData)
		{
			size_t indexCount = primitiveData.IndicesStream.size();
			size_t vertexCount = primitiveData.VerticesStream.size();

			std::vector<uint32> remap(vertexCount);
			meshopt_optimizeVertexFetchRemap(&remap[0], primitiveData.IndicesStream.data(), indexCount, vertexCount);

			meshopt_remapIndexBuffer(primitiveData.IndicesStream.data(), primitiveData.IndicesStream.data(), indexCount, remap.data());
			meshopt_remapVertexBuffer(primitiveData.VerticesStream.data(), primitiveData.VerticesStream.data(), vertexCount, sizeof(MeshVertex), remap.data());
			meshopt_optimizeVertexCache(primitiveData.IndicesStream.data(), primitiveData.IndicesStream.data(), indexCount, vertexCount);
			meshopt_optimizeOverdraw(primitiveData.IndicesStream.data(), primitiveData.IndicesStream.data(), indexCount, &primitiveData.VerticesStream[0].Position.x, vertexCount, sizeof(MeshVertex), 1.05f);
		}

		void CreateMeshlets(PrimitiveData& data)
		{
			size_t indexCount = data.IndicesStream.size();
			size_t vertexCount = data.VerticesStream.size();

			size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES);

			std::vector<meshopt_Meshlet> meshlets(maxMeshlets);
			std::vector<uint8>indices(maxMeshlets * MESHLET_MAX_TRIANGLES * 3);
			data.MeshletVertices.resize(maxMeshlets * MESHLET_MAX_VERTICES);
			size_t meshletCount = meshopt_buildMeshlets(meshlets.data(), data.MeshletVertices.data(), indices.data(),
														data.IndicesStream.data(), indexCount, &data.VerticesStream[0].Position.x, vertexCount,
														sizeof(MeshVertex), MESHLET_MAX_VERTICES, MESHLET_MAX_TRIANGLES, 0.0f);

			// Trimming
			const meshopt_Meshlet& last = meshlets[meshletCount - 1];

			meshlets.resize(meshletCount);
			indices.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
			data.Meshlets.reserve(meshletCount);
			data.MeshletVertices.resize(last.vertex_offset + last.vertex_count);
			data.MeshletTriangles.resize(indices.size() / 3);

			uint32 triangleOffset = 0;
			for (const meshopt_Meshlet& meshlet : meshlets)
			{
				uint8* pData = indices.data() + meshlet.triangle_offset;
				for (uint32 i = 0; i < meshlet.triangle_count; ++i)
				{
					Meshlet::Triangle& triangle = data.MeshletTriangles[i + triangleOffset];
					triangle.V0 = *pData++;
					triangle.V1 = *pData++;
					triangle.V2 = *pData++;
				}

				data.Meshlets.emplace_back(meshlet.vertex_offset, triangleOffset, meshlet.vertex_count, meshlet.triangle_count);
				triangleOffset += meshlet.triangle_count;
			}
			data.MeshletTriangles.resize(triangleOffset);
		}

		void CalculateNormals(PrimitiveData& data)
		{
			data.NormalsStream.resize(data.PositionStream.size(), float3(0.0f));

			for (int i = 0; i < data.IndicesStream.size(); ++i)
			{
				const uint idx0 = data.IndicesStream[i + 0];
				const uint idx1 = data.IndicesStream[i + 1];
				const uint idx2 = data.IndicesStream[i + 2];

				const float3 p0 = data.PositionStream[idx0];
				const float3 p1 = data.PositionStream[idx1];
				const float3 p2 = data.PositionStream[idx2];

				const float3 v0 = glm::normalize(p0 - p1);
				const float3 v1 = glm::normalize(p1 - p2);

				float3 normal = glm::normalize(glm::cross(v0, v1));
				ensure(!glm::any(glm::isnan(normal)));

				data.NormalsStream[i + 0] = normal;
				data.NormalsStream[i + 1] = normal;
				data.NormalsStream[i + 2] = normal;
			}
		}

		void CalculateTangents(PrimitiveData& data)
		{
			// https://terathon.com/blog/tangent-space.html
			std::vector<float3> bitangents, tangents;
			bitangents.resize(data.PositionStream.size(), float3(0.0f));
			tangents.resize(data.PositionStream.size(), float3(0.0f));

			for (int i = 0; i < data.IndicesStream.size(); ++i)
			{
				const uint idx0 = data.IndicesStream[i + 0];
				const uint idx1 = data.IndicesStream[i + 1];
				const uint idx2 = data.IndicesStream[i + 2];

				const float3 p0 = data.PositionStream[idx0];
				const float3 p1 = data.PositionStream[idx1];
				const float3 p2 = data.PositionStream[idx2];

				const float2 uv0 = data.TexCoordsStream[idx0];
				const float2 uv1 = data.TexCoordsStream[idx1];
				const float2 uv2 = data.TexCoordsStream[idx2];

				const float3 q1 = p1 - p0;
				const float3 q2 = p2 - p0;

				const float s1 = uv1.x - uv0.x;
				const float s2 = uv2.x - uv0.x;

				const float t1 = uv1.y - uv0.y;
				const float t2 = uv2.y - uv0.y;

				const float r = 1 / ((s1 * t2) - (s2 * t1));

				float3 t = float3((t2 * q1 - t1 * q2) * r);
				float3 b = float3((s1 * q2 - s2 * q1) * r);
				
				tangents[idx0] += t;
				tangents[idx1] += t;
				tangents[idx2] += t;
			}

			data.TangentsStream.resize(data.PositionStream.size(), float4(0.0f));
			for (int i = 0; i < data.IndicesStream.size(); ++i)
			{
				const float3 t = tangents[i];
				const float3 b = bitangents[i];
				const float3 n = data.NormalsStream[i];

				// Gram-Schmidt process to make sure the vectors are perpendicular to each other. Here is a nice explanation of it: https://youtu.be/4FaWLgsctqY?si=TlqfkJl2AtK3cNxJ&t=1218
				float3 tangent = t - glm::dot(t, n) * n;

				data.TangentsStream[i] = float4(tangent, 1.0f);
			}
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
		std::vector<PrimitiveData>().swap(PrimitivesStreams);
		std::unordered_map<uintptr_t, uint32>().swap(TexturesMap);
		for (TextureData& texture : TextureStreams)
		{
			free(texture.Data);
			texture.Data = nullptr;
		}
		std::vector<TextureData>().swap(TextureStreams);
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
				material.AlbedoIndex  = CreateTextureResource(&workflow.base_color_texture, debugName.c_str(), true);
				material.AlbedoFactor = float4(workflow.base_color_factor[0], workflow.base_color_factor[1], workflow.base_color_factor[2], workflow.base_color_factor[3]);
			}
			{
				std::string debugName = std::format(" Material({}) {}", index, "MetallicRoughness");
				material.RoughnessMetalIndex = CreateTextureResource(&workflow.metallic_roughness_texture, debugName.c_str(), false);
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
			material.NormalIndex = CreateTextureResource(&cgltfMaterial->normal_texture, debugName.c_str(), false);
		}

		{
			std::string debugName   = std::format(" Material({}) {}", index, "Emissive");
			material.EmissiveIndex  = CreateTextureResource(&cgltfMaterial->emissive_texture, debugName.c_str(), false);
			material.EmissiveFactor = float3(cgltfMaterial->emissive_factor[0], cgltfMaterial->emissive_factor[1], cgltfMaterial->emissive_factor[2]);
		}

		{
			std::string debugName			= std::format(" Material({}) {}", index, "AmbientOcclusion");
			material.AmbientOcclusionIndex  = CreateTextureResource(&cgltfMaterial->occlusion_texture, debugName.c_str(), false);
		}
	}


	void Scene::LoadTexture(const cgltf_texture* texture)
	{
		if (!texture)
			return;

		TextureData data;

		cgltf_image* image = texture->image;
		if (image->uri)
		{
			data.Name = image->uri;
			std::string filename = std::string(m_FolderPath) + std::string(image->uri);
			if (filename.find(".dds") != std::string::npos)
			{
				std::vector<uint8> filedata;
				check(Utils::FileRead(filename.c_str(), filedata));
				dds::Header header = dds::read_header(filedata.data(), filedata.size());
				check(header.is_valid());

				uint8* textureData = filedata.data() + header.data_offset(); 
				data.Data = malloc(filedata.size());
				memcpy(data.Data, textureData, filedata.size());

				data.Width = header.width();
				data.Height = header.height();
				data.NumMips = header.mip_levels();
				data.bGenerateMips = false;
				data.Format = RHI::GetFormat((DXGI_FORMAT)header.format());
			}
			else
			{
				data.Data = stbi_load(filename.c_str(), &data.Width, &data.Height, &data.Channels, 4);
				data.NumMips = RHI::CalculateMipCount(data.Width);
				data.bGenerateMips = true;
				data.Format = RHI::Format::RGBA8_UNORM;
			}
		}
		else
		{
			data.Name = m_SceneName;
			cgltf_buffer_view* bufferView = image->buffer_view;
			cgltf_buffer* buffer = bufferView->buffer;
			uint32 size = (uint32)bufferView->size;
			void* bufferLocation = (uint8*)buffer->data + bufferView->offset;
			data.Data = stbi_load_from_memory((stbi_uc*)bufferLocation, size, &data.Width, &data.Height, &data.Channels, 4);
			data.NumMips = RHI::CalculateMipCount(data.Width);
			data.bGenerateMips = true;
			data.Format = RHI::Format::RGBA8_UNORM;
		}
		ensure(data.Data);

		std::scoped_lock<std::mutex> lock(m_AddToTextureMapMutex);
		TexturesMap[(uintptr_t)texture] = (uint32)TextureStreams.size();
		TextureStreams.emplace_back(data);
	}

	uint Scene::CreateTextureResource(const cgltf_texture_view* textureView, const std::string& debugName, bool bIsSRGB)
	{
		if (!textureView->texture)
			return -1;

		TextureData& textureData = TextureStreams.at(TexturesMap[(uintptr_t)textureView->texture]);
		check(textureData.Data);
		textureData.Name += debugName;

		uint16 numMips = textureData.bGenerateMips ? 1u : textureData.NumMips;
		RHI::TextureUsage usage = RHI::TextureUsage::ShaderResource;
		if (textureData.bGenerateMips)
			usage |= RHI::TextureUsage::UnorderedAccess;

		RHI::Format format = bIsSRGB ? RHI::ConvertToSRGBFormat(textureData.Format) : textureData.Format;

		RHI::TextureHandle texture = RHI::CreateTexture({
			.Width = (uint32)textureData.Width,
			.Height = (uint32)textureData.Height,
			.MipLevels = textureData.NumMips,
			.DebugName = textureData.Name.c_str(),
			.Flags = usage,
			.Format = format,
			.Type = RHI::TextureType::Texture2D,
			.InitialData = {
				.Data = textureData.Data,
				.NumMips = numMips
			}
		});

		if (textureData.bGenerateMips)
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
			readAttributeData("TANGENT", primitiveData.TangentsStream, 4);
			readAttributeData("TEXCOORD_0", primitiveData.TexCoordsStream, 2);
		}

		if (primitiveData.NormalsStream.empty() && !primitiveData.PositionStream.empty())
			CalculateNormals(primitiveData);

		if (primitiveData.TangentsStream.empty() && !primitiveData.TexCoordsStream.empty())
			CalculateTangents(primitiveData);

		// process indices
		cgltf_accessor* indices = primitive->indices;
		primitiveData.IndicesStream.resize(indices->count);
		for (size_t idx = 0; idx < indices->count; ++idx)
			primitiveData.IndicesStream[idx] = (uint32)cgltf_accessor_read_index(primitive->indices, idx);

		cgltf_material* material = primitive->material;
		Mesh& result = m_Meshes.emplace_back();
		cgltf_node_transform_world(node, &result.Transform[0][0]);
		result.LocalMaterialIndex = m_MaterialPtrToIndex[material];
		result.bIsOpaque = material ? material->alpha_mode == cgltf_alpha_mode_opaque : true;
		result.Name = meshName.c_str();
	}

	void Scene::ProcessPrimitivesData()
	{
		uint64 bufferSize = 0;
		uint32 elementCount = 0;

		for (size_t i = 0; i < PrimitivesStreams.size(); ++i)
		{
			CreateVertexStream(PrimitivesStreams[i]);
			
			// Optimize the mesh data and create the meshlets
			OptimizePrimitiveData(PrimitivesStreams[i]);
			CreateMeshlets(PrimitivesStreams[i]);

			// Calculate the geometry buffer size
			bufferSize += PrimitivesStreams[i].VerticesStream.size()   * sizeof(MeshVertex);
			bufferSize += PrimitivesStreams[i].IndicesStream.size()    * sizeof(uint32);
			bufferSize += PrimitivesStreams[i].Meshlets.size()		   * sizeof(Meshlet);
			bufferSize += PrimitivesStreams[i].MeshletVertices.size()  * sizeof(uint32);
			bufferSize += PrimitivesStreams[i].MeshletTriangles.size() * sizeof(Meshlet::Triangle);
		}
		check(bufferSize <= 0xffffffffu);

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

		RHI::Buffer* pUploadBuffer = RM_GET(upload);
		pUploadBuffer->Map();
		uint8* data = (uint8*)pUploadBuffer->MappedData;
		uint64 dataOffset = 0;
		for (size_t i = 0; i < PrimitivesStreams.size(); ++i)
		{
			check(dataOffset % sizeof(uint32) == 0); // the offset is a 32bit value, do not let it overflow

			CopyVertexData(m_Meshes[i].VerticesLocation, data, geoBufferAddress, dataOffset, PrimitivesStreams[i].VerticesStream);

			CopyMeshletData(m_Meshes[i].MeshletsOffset, data, dataOffset, PrimitivesStreams[i].Meshlets);
			CopyMeshletData(m_Meshes[i].MeshletVerticesOffset, data, dataOffset, PrimitivesStreams[i].MeshletVertices);
			CopyMeshletData(m_Meshes[i].MeshletTrianglesOffset, data, dataOffset, PrimitivesStreams[i].MeshletTriangles);

			size_t streamSize = PrimitivesStreams[i].IndicesStream.size() * sizeof(uint32);
			m_Meshes[i].IndicesLocation = {
				.BufferLocation = geoBufferAddress + dataOffset,
				.SizeInBytes = (uint32)streamSize,
				.Offset = (uint32)dataOffset
			};
			memcpy(data + dataOffset, PrimitivesStreams[i].IndicesStream.data(), streamSize);
			dataOffset += streamSize;

			m_Meshes[i].IndexCount    = PrimitivesStreams[i].IndicesStream.size();
			m_Meshes[i].VertexCount   = PrimitivesStreams[i].VerticesStream.size();
			m_Meshes[i].MeshletsCount = PrimitivesStreams[i].Meshlets.size();
		}
		check(dataOffset <= 0xffffffffu);

		RHI::CommandContext::GetCommandContext()->CopyBufferToBuffer(upload, m_GeometryBuffer, bufferSize);
		RHI::CommandContext::GetCommandContext()->InsertResourceBarrier(m_GeometryBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_INDEX_BUFFER | D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
		DestroyBuffer(upload);
	}
}
