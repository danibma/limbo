#include "stdafx.h"
#include "ringbufferallocator.h"
#include "resourcemanager.h"
#include "commandqueue.h"
#include "device.h"

namespace limbo::Gfx
{
	RingBufferAllocator::RingBufferAllocator(uint64 size)
		: m_TotalSize(size), m_CurrentOffset(0)
	{
		m_Buffer		= CreateBuffer({ .DebugName = "Ring Buffer Allocator", .ByteSize = size, .Flags = BufferUsage::Upload });
		Map(m_Buffer);
		m_MappedData	= GetMappedData(m_Buffer);

		m_Queue			= Device::Ptr->GetCommandQueue(ContextType::Copy);
	}

	RingBufferAllocator::~RingBufferAllocator()
	{
		m_Queue->WaitForIdle();

		DestroyBuffer(m_Buffer);
	}

	void RingBufferAllocator::Allocate(uint64 size, RingBufferAllocation& allocation)
	{
		FAILIF(size > m_TotalSize);

		uint64 freedOffset = 0;

		while(!m_PreDeletedList.empty())
		{
			PreDeletedAllocation& deleted = m_PreDeletedList.front();
			if (!m_Queue->GetFence()->IsComplete(deleted.FenceValue))
			{
				break;
			}
			freedOffset = deleted.Offset + deleted.Size;
			m_PreDeletedList.pop();
		}

		constexpr uint64 InvalidOffset = 0xFFFFFFFF;
		uint64 offset = InvalidOffset;

		if (m_CurrentOffset + size <= m_TotalSize)
		{
			offset = m_CurrentOffset;
			m_CurrentOffset += size;
		}
		else if (size <= freedOffset)
		{
			offset = 0;
			m_CurrentOffset = size;
		}

		FAILIF(offset == InvalidOffset);

		allocation.Context		= GetCommandContext(ContextType::Copy);
		allocation.Buffer		= GetBuffer(m_Buffer);
		allocation.Offset		= offset;
		allocation.Size			= size;
		allocation.MappedData	= (uint8*)m_MappedData + offset;
	}

	void RingBufferAllocator::Free(RingBufferAllocation& allocation)
	{
		uint64 fenceValue = allocation.Context->Execute();
		m_PreDeletedList.emplace(fenceValue, allocation.Offset, allocation.Size);
	}
}
