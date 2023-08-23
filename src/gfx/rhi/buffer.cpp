#include "stdafx.h"
#include "buffer.h"
#include "device.h"

#include "core/utils.h"

namespace limbo::Gfx
{
	Buffer::Buffer(const BufferSpec& spec)
		: CurrentState(D3D12_RESOURCE_STATE_COMMON), ByteStride(spec.ByteStride), ByteSize(spec.ByteSize)
	{
		Device* device = Device::Ptr;
		ID3D12Device* d3ddevice = device->GetDevice();

		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
		if (EnumHasAllFlags(spec.Flags, BufferUsage::AccelerationStructure))
			flags |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		uint64 alignment = 4;
		if (EnumHasAnyFlags(spec.Flags, BufferUsage::Upload | BufferUsage::Constant))
			alignment = 256;

		D3D12_RESOURCE_DESC desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
			.Width = Math::Max(Math::Align(spec.ByteSize, alignment), alignment),
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {
				.Count = 1,
				.Quality = 0
			},
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = flags
		};

		D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
		if (EnumHasAllFlags(spec.Flags, BufferUsage::Upload))
		{
			heapType = D3D12_HEAP_TYPE_UPLOAD;
			CurrentState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}

		if (EnumHasAllFlags(spec.Flags, BufferUsage::AccelerationStructure | BufferUsage::ShaderResourceView))
			CurrentState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		// #todo: use CreatePlacedResource instead of CreateCommittedResource
		D3D12_HEAP_PROPERTIES heapProps = {
			.Type = heapType,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
		};

		InitialState = CurrentState;
		DX_CHECK(d3ddevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
													InitialState,
													nullptr,
													IID_PPV_ARGS(&Resource)));

		if (spec.InitialData)
		{
			if (EnumHasAllFlags(spec.Flags, BufferUsage::Upload))
			{
				DX_CHECK(Resource->Map(0, nullptr, &MappedData));
				memcpy(MappedData, spec.InitialData, spec.ByteSize);
			}
			else
			{
				std::string uploadName = std::string(spec.DebugName) + " (Upload Buffer)";
				Handle<Buffer> uploadBuffer = CreateBuffer({
					.DebugName = uploadName.c_str(),
					.ByteSize = spec.ByteSize,
					.Flags = BufferUsage::Upload,
					.InitialData = spec.InitialData
				});
				Buffer* allocationBuffer = ResourceManager::Ptr->GetBuffer(uploadBuffer);
				FAILIF(!allocationBuffer);
				Device::Ptr->GetCommandContext(ContextType::Direct)->CopyBufferToBuffer(allocationBuffer, this, spec.ByteSize, 0, 0);
				DestroyBuffer(uploadBuffer);
			}
		}

		if ((spec.DebugName != nullptr) && (spec.DebugName[0] != '\0'))
		{
			std::wstring wname;
			Utils::StringConvert(spec.DebugName, wname);
			DX_CHECK(Resource->SetName(wname.c_str()));
		}

		InitResource(spec);
	}

	Buffer::~Buffer()
	{
	}

	void Buffer::CreateCBV(ID3D12Device* device, const BufferSpec& spec)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
			.BufferLocation = Resource->GetGPUVirtualAddress(),
			.SizeInBytes = Math::Max(Math::Align((uint32)spec.ByteSize, (uint32)256), (uint32)256ul)
		};

		device->CreateConstantBufferView(&cbvDesc, BasicHandle.CpuHandle);
	}

	void Buffer::CreateSRV(ID3D12Device* device, const BufferSpec& spec)
	{
		bool bIsRaw = EnumHasAllFlags(spec.Flags, BufferUsage::Byte);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
			.ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};
		srvDesc.Buffer.FirstElement = 0;

		if (bIsRaw)
		{
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.Buffer.NumElements = spec.NumElements;
			srvDesc.Buffer.StructureByteStride = 0;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		}
		else
		{
			srvDesc.Format = DXGI_FORMAT_UNKNOWN;
			srvDesc.Buffer.NumElements = spec.NumElements;
			srvDesc.Buffer.StructureByteStride = spec.ByteStride;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		}

		device->CreateShaderResourceView(Resource.Get(), &srvDesc, BasicHandle.CpuHandle);
	}

	void Buffer::CreateSRV_AS(ID3D12Device* device, const BufferSpec& spec)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
			.Format = DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE,
			.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING
		};

		srvDesc.RaytracingAccelerationStructure = {
			.Location = Resource->GetGPUVirtualAddress()
		};

		device->CreateShaderResourceView(nullptr, &srvDesc, BasicHandle.CpuHandle);
	}

	void Buffer::InitResource(const BufferSpec& spec)
	{
		if (EnumHasAllFlags(spec.Flags, BufferUsage::Constant))
		{
			BasicHandle = Device::Ptr->AllocateHandle(DescriptorHeapType::SRV);
			CreateCBV(Device::Ptr->GetDevice(), spec);
		}
		else if (EnumHasAllFlags(spec.Flags, BufferUsage::ShaderResourceView))
		{
			BasicHandle = Device::Ptr->AllocateHandle(DescriptorHeapType::SRV);
			if (EnumHasAllFlags(spec.Flags, BufferUsage::AccelerationStructure))
				CreateSRV_AS(Device::Ptr->GetDevice(), spec);
			else
				CreateSRV(Device::Ptr->GetDevice(), spec);
		}
	}
}
