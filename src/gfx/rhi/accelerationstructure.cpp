#include "stdafx.h"
#include "accelerationstructure.h"
#include "device.h"
#include "gfx/scene.h"
#include "commandcontext.h"

namespace limbo::RHI
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

	void AccelerationStructure::Build(CommandContext* cmd, const std::vector<Gfx::Scene*>& scenes)
	{
		if (!RHI::GetGPUInfo().bSupportsRaytracing) return;

		Device* device = Device::Ptr;
		ID3D12Device5* d3ddevice = device->GetDevice();

		cmd->BeginProfileEvent("Build Acceleration Structure");

		bool bUpdateBLAS = false;
		std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances;
		for (Gfx::Scene* scene : scenes)
		{
			instances.reserve(instances.capacity() + scene->NumMeshes());
			scene->IterateMeshesNoConst(Gfx::TOnDrawMeshNoConst::CreateLambda([&](Gfx::Mesh& mesh)
			{
				if (!mesh.BLAS.IsValid())
				{
					// Describe the geometry.
					D3D12_RAYTRACING_GEOMETRY_DESC geometry;
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					geometry.Triangles.VertexBuffer.StartAddress = mesh.VerticesLocation.BufferLocation;
					geometry.Triangles.VertexBuffer.StrideInBytes = mesh.VerticesLocation.StrideInBytes;
					geometry.Triangles.VertexCount = (uint32)mesh.VertexCount;
					geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
					geometry.Triangles.IndexBuffer = mesh.IndicesLocation.BufferLocation;
					geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
					geometry.Triangles.IndexCount = (uint32)mesh.IndexCount;
					geometry.Triangles.Transform3x4 = 0;
					geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; // todo: check if a mesh has need for alpha testing, if not, use the opaque flag

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

					BufferHandle blasScratch = CreateBuffer({
						.DebugName = "BLAS Scratch Buffer",
						.ByteSize = ASBuildInfo.ScratchDataSizeInBytes,
						.Flags = BufferUsage::AccelerationStructure,
					});
					BufferHandle blasResult = CreateBuffer({
						.DebugName = "BLAS Result Buffer",
						.ByteSize = ASBuildInfo.ResultDataMaxSizeInBytes,
						.Flags = BufferUsage::AccelerationStructure | BufferUsage::ShaderResourceView,
					});

					cmd->BuildRaytracingAccelerationStructure(ASInputs, blasScratch, blasResult);
					cmd->InsertUAVBarrier(blasResult);
					DestroyBuffer(blasScratch);
					mesh.BLAS = blasResult;
					bUpdateBLAS = true;
				}

				Buffer* pResult = RM_GET(mesh.BLAS);

				// Describe the top-level acceleration structure instance(s).
				D3D12_RAYTRACING_INSTANCE_DESC& instance = instances.emplace_back();
				memcpy(instance.Transform, &glm::transpose(mesh.Transform)[0], sizeof(float3x4));
				instance.InstanceID = 0;
				instance.InstanceMask = 0xFF;
				instance.InstanceContributionToHitGroupIndex = 0;
				instance.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
				instance.AccelerationStructure = pResult->Resource->GetGPUVirtualAddress();
			}));
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
		TLASInput.InstanceDescs = RM_GET(m_InstancesBuffer)->Resource->GetGPUVirtualAddress();
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
		cmd->BuildRaytracingAccelerationStructure(TLASInput, m_ScratchBuffer, m_TLAS);
		cmd->InsertUAVBarrier(m_TLAS);

		cmd->EndProfileEvent("Build Acceleration Structure");
	}

	Buffer* AccelerationStructure::GetTLASBuffer() const
	{
		return RM_GET(m_TLAS);
	}
}
