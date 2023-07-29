#include "ringbufferallocator.h"
#include "device.h"
#include "resourcemanager.h"
#include "gfx/gfx.h"

namespace limbo::gfx
{
	RingBufferAllocator* RingBufferAllocator::ptr = nullptr;

	RingBufferAllocator::RingBufferAllocator(uint64 size)
		: m_maxSize(size), m_currentOffset(0), m_mappedData(nullptr)
	{
		onPostResourceManagerInit.AddLambda([&]()
		{
			if (!m_buffer.isValid())
			{
				m_buffer = createBuffer({
					.debugName = "Ring Buffer",
					.byteSize = m_maxSize,
					.usage = BufferUsage::Upload,
					});

				Buffer* b = ResourceManager::ptr->getBuffer(m_buffer);
				DX_CHECK(b->resource->Map(0, nullptr, &m_mappedData));
			}
		});

		onPreResourceManagerShutdown.AddLambda([&]()
		{
			destroyBuffer(m_buffer);
		});
	}

	RingBufferAllocator::~RingBufferAllocator()
	{
	}

	bool RingBufferAllocator::allocate(uint64 size, RingBufferAllocation& allocation)
	{
		if (m_currentOffset + size > m_maxSize)
			return false;

		allocation.buffer		= m_buffer;
		allocation.offset		= m_currentOffset;
		allocation.mappedData	= (uint8*)m_mappedData + m_currentOffset;

		m_currentOffset += size;

		return true;
	}
}
