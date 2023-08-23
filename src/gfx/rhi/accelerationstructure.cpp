#include "stdafx.h"
#include "accelerationstructure.h"

#include <glm/gtc/type_ptr.inl>

#include "device.h"
#include "gfx/scene.h"

namespace limbo::Gfx
{
	AccelerationStructure::~AccelerationStructure()
	{
		if (m_ScratchBuffer.IsValid())
			DestroyBuffer(m_ScratchBuffer);
		if (m_TLAS.IsValid())
			DestroyBuffer(m_TLAS);
		if (m_InstancesBuffer.IsValid())
			DestroyBuffer(m_InstancesBuffer);
	}

	void AccelerationStructure::Build(const std::vector<Scene*>& scenes)
	{
		Device* device = Device::Ptr;
		ID3D12Device5* d3ddevice = device->GetDevice();

		BeginEvent("Build Acceleration Structure");

		bool bUpdateBLAS = false;
		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances;
		for (Scene* scene : scenes)
		{
			instances.reserve(instances.capacity() + scene->NumMeshes());
			scene->IterateMeshesNoConst([&](Mesh& mesh)
			{
				if (!mesh.BLAS.IsValid())
				{
					// Describe the geometry.
					D3D12_RAYTRACING_GEOMETRY_DESC geometry;
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					geometry.Triangles.VertexBuffer.StartAddress = mesh.PositionsLocation.BufferLocation;
					geometry.Triangles.VertexBuffer.StrideInBytes = mesh.PositionsLocation.StrideInBytes;
					geometry.Triangles.VertexCount = (uint32)mesh.VertexCount;
					geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
					geometry.Triangles.IndexBuffer = mesh.IndicesLocation.BufferLocation;
					geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
					geometry.Triangles.IndexCount = (uint32)mesh.IndexCount;
					geometry.Triangles.Transform3x4 = 0;
					geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE; // todo: check if a mesh has need for alpha testing, if not, use the opaque flag

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
						.Flags = BufferUsage::AccelerationStructure,
					});
					Handle<Buffer> blasResult = CreateBuffer({
						.DebugName = "BLAS Result Buffer",
						.ByteSize = ASBuildInfo.ResultDataMaxSizeInBytes,
						.Flags = BufferUsage::AccelerationStructure | BufferUsage::ShaderResourceView,
					});

					device->GetCommandContext(ContextType::Direct)->BuildRaytracingAccelerationStructure(ASInputs, blasScratch, blasResult);
					device->GetCommandContext(ContextType::Direct)->UAVBarrier(blasResult);
					DestroyBuffer(blasScratch);
					mesh.BLAS = blasResult;
					bUpdateBLAS = true;
				}

				Buffer* pResult = GetBuffer(mesh.BLAS);

				// Describe the top-level acceleration structure instance(s).
				D3D12_RAYTRACING_INSTANCE_DESC& instance = instances.emplace_back();
				memcpy(instance.Transform, glm::value_ptr(glm::transpose(mesh.Transform)), sizeof(float3x4));
				instance.InstanceID = 0;
				instance.InstanceMask = 1;
				instance.InstanceContributionToHitGroupIndex = 0;
				instance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				instance.AccelerationStructure = pResult->Resource->GetGPUVirtualAddress();
			});
		}

		if (!m_InstancesBuffer.IsValid() || bUpdateBLAS)
		{
			// Delete the _old_ buffers if we are updating the instances
			if (m_InstancesBuffer.IsValid())
			{
				DestroyBuffer(m_InstancesBuffer);
				DestroyBuffer(m_TLAS);
				DestroyBuffer(m_ScratchBuffer);
			}

			// Build BLAS instances buffer
			m_InstancesBuffer = CreateBuffer({
				.DebugName = "BLAS Instances Buffer",
				.ByteSize = instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC),
				.Flags = BufferUsage::Upload,
				.InitialData = instances.data()
			});
		}

		// Build TLAS
		// Describe the bottom-level acceleration structure inputs.
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS TLASInput = {};
		TLASInput.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		TLASInput.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		TLASInput.InstanceDescs = GetBuffer(m_InstancesBuffer)->Resource->GetGPUVirtualAddress();
		TLASInput.NumDescs = (uint32)instances.size();
		TLASInput.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		if (!m_TLAS.IsValid() || bUpdateBLAS)
		{
			// Get the memory requirements to build the BLAS.
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASBuildInfo = {};
			d3ddevice->GetRaytracingAccelerationStructurePrebuildInfo(&TLASInput, &ASBuildInfo);

			m_ScratchBuffer = CreateBuffer({
				.DebugName = "TLAS Scratch Buffer",
				.ByteSize = ASBuildInfo.ScratchDataSizeInBytes,
				.Flags = BufferUsage::AccelerationStructure,
			});

			m_TLAS = CreateBuffer({
				.DebugName = "TLAS Result Buffer",
				.ByteSize = ASBuildInfo.ResultDataMaxSizeInBytes,
				.Flags = BufferUsage::AccelerationStructure | BufferUsage::ShaderResourceView,
			});
		}

		// build top level acceleration structure
		device->GetCommandContext(ContextType::Direct)->BuildRaytracingAccelerationStructure(TLASInput, m_ScratchBuffer, m_TLAS);
		device->GetCommandContext(ContextType::Direct)->UAVBarrier(m_TLAS);

		EndEvent();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE AccelerationStructure::GetDescriptor() const
	{
		return GetBuffer(m_TLAS)->BasicHandle.GPUHandle;
	}
}
