﻿#include "stdafx.h"
#include "accelerationstructure.h"

#include <glm/gtc/type_ptr.inl>

#include "device.h"
#include "gfx/scene.h"

namespace limbo::Gfx
{
	AccelerationStructure::~AccelerationStructure()
	{
		DestroyBuffer(m_ScratchBuffer);
		DestroyBuffer(m_TLAS);
	}

	void AccelerationStructure::Build(const std::vector<Scene*>& scenes)
	{
		Device* device = Device::Ptr;
		ID3D12Device5* d3ddevice = device->GetDXRDevice();

		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances;

		for (const Scene* scene : scenes)
		{
			scene->IterateMeshes([&](const Mesh& mesh)
			{
				device->TransitionResource(mesh.VertexBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				device->TransitionResource(mesh.IndexBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

				ID3D12Resource* vb = ResourceManager::Ptr->GetBuffer(mesh.VertexBuffer)->Resource.Get();
				ID3D12Resource* ib = ResourceManager::Ptr->GetBuffer(mesh.IndexBuffer)->Resource.Get();

				// Describe the geometry.
				D3D12_RAYTRACING_GEOMETRY_DESC geometry;
				geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
				geometry.Triangles.VertexBuffer.StartAddress = vb->GetGPUVirtualAddress();
				geometry.Triangles.VertexBuffer.StrideInBytes = sizeof(MeshVertex);
				geometry.Triangles.VertexCount = (uint32)mesh.VertexCount;
				geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
				geometry.Triangles.IndexBuffer = ib->GetGPUVirtualAddress();
				geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
				geometry.Triangles.IndexCount = (uint32)mesh.IndexCount;
				geometry.Triangles.Transform3x4 = 0;
				geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

				// Describe the bottom-level acceleration structure inputs.
				D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
				ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
				ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
				ASInputs.pGeometryDescs = &geometry;
				ASInputs.NumDescs = 1;
				ASInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

				// Get the memory requirements to build the BLAS.
				D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASBuildInfo = {};
				d3ddevice->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASBuildInfo);

				Handle<Buffer> blasScratch = CreateBuffer({ 
					.DebugName = "BLAS Scratch Buffer",
					.ByteSize = ASBuildInfo.ScratchDataSizeInBytes,
					.Usage = BufferUsage::AS_Scratch,
				});
				Handle<Buffer> blasResult = CreateBuffer({ 
					.DebugName = "BLAS Result Buffer",
					.ByteSize = ASBuildInfo.ResultDataMaxSizeInBytes,
					.Usage = BufferUsage::AS_Result,
				});

				device->BuildRaytracingAccelerationStructure(ASInputs, blasScratch, blasResult);
				device->UAVBarrier(blasResult);

				Buffer* pResult = GetBuffer(blasResult);
				// Describe the top-level acceleration structure instance(s).
				D3D12_RAYTRACING_INSTANCE_DESC& instance = instances.emplace_back();
				//memcpy(instance.Transform, glm::value_ptr(glm::mat3x4(mesh.Transform)), sizeof(glm::mat3x4));
				instance.Transform[0][0] = instance.Transform[1][1] = instance.Transform[2][2] = 1;
				instance.InstanceID = 0;
				instance.InstanceMask = 1;
				instance.InstanceContributionToHitGroupIndex = 0;
				instance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				instance.AccelerationStructure = pResult->Resource->GetGPUVirtualAddress();

				DestroyBuffer(blasScratch);
				DestroyBuffer(blasResult);
			});
		}

		// Build BLAS instances buffer
		Handle<Buffer> blasInstancesBuffer = CreateBuffer({
			.DebugName = "BLAS Instances Buffer",
			.ByteSize = instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
			.Usage = BufferUsage::Upload,
			.InitialData = instances.data()
		});

		// Build TLAS
		// Describe the bottom-level acceleration structure inputs.
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS TLASInput = {};
		TLASInput.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		TLASInput.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		TLASInput.InstanceDescs = GetBuffer(blasInstancesBuffer)->Resource->GetGPUVirtualAddress();
		TLASInput.NumDescs = (uint32)instances.size();
		TLASInput.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		if (!m_TLAS.IsValid())
		{
			// Get the memory requirements to build the BLAS.
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASBuildInfo = {};
			d3ddevice->GetRaytracingAccelerationStructurePrebuildInfo(&TLASInput, &ASBuildInfo);

			m_ScratchBuffer = CreateBuffer({
				.DebugName = "TLAS Scratch Buffer",
				.ByteSize = ASBuildInfo.ScratchDataSizeInBytes,
				.Usage = BufferUsage::AS_Scratch,
			});

			m_TLAS = CreateBuffer({
				.DebugName = "TLAS Result Buffer",
				.ByteSize = ASBuildInfo.ResultDataMaxSizeInBytes,
				.Usage = BufferUsage::AS_Result,
				.bCreateView = true
			});
		}

		// build top level acceleration structure
		device->BuildRaytracingAccelerationStructure(TLASInput, m_ScratchBuffer, m_TLAS);
		device->UAVBarrier(m_TLAS);

		DestroyBuffer(blasInstancesBuffer);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE AccelerationStructure::GetDescriptor() const
	{
		return GetBuffer(m_TLAS)->BasicHandle.GPUHandle;
	}
}
