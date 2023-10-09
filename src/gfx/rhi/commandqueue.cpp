#include "stdafx.h"
#include "commandqueue.h"
#include "commandcontext.h"
#include "device.h"
#include "core/utils.h"

namespace limbo::RHI
{
	Fence::Fence(Device* device)
	{
		DX_CHECK(device->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetAddressOf())));
		m_CurrentValue  = 0;

		// Create an event handler to use for frame synchronization
		m_CompleteEvent = CreateEvent(nullptr, false, false, nullptr);
		ensure(m_CompleteEvent);
	}

	Fence::~Fence()
	{
		CloseHandle(m_CompleteEvent);
	}

	uint64 Fence::Signal(CommandQueue* queue)
	{
		DX_CHECK(queue->Get()->Signal(m_Fence.Get(), m_CurrentValue));
		m_CurrentValue++;
		return m_CurrentValue - 1;
	}

	void Fence::CpuWait(uint64 fenceValue)
	{
		if (IsComplete(fenceValue))
			return;

		m_Fence->SetEventOnCompletion(fenceValue, m_CompleteEvent);
		DWORD result = WaitForSingleObject(m_CompleteEvent, INFINITE);

		if (result == WAIT_OBJECT_0)
			PIXNotifyWakeFromFenceSignal(m_CompleteEvent);
	}

	void Fence::CpuWait()
	{
		CpuWait(m_CurrentValue);
	}

	bool Fence::IsComplete(uint64 fenceValue)
	{
		return fenceValue <= m_Fence->GetCompletedValue();
	}

	CommandQueue::CommandQueue(ContextType type, Device* device)
		: m_Type(type)
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {
			.Type = D3DCmdListType(m_Type),
			.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
			.NodeMask = 0
		};

		DX_CHECK(device->GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_Queue.ReleaseAndGetAddressOf())));

		std::string name = std::format("{} Command queue", CmdListTypeToStr(m_Type));
		std::wstring wName;
		Utils::StringConvert(name, wName);
		DX_CHECK(m_Queue->SetName(wName.c_str()));

		m_Fence = new Fence(device);

		for (int i = 0; i < NUM_BACK_BUFFERS + 1; ++i)
		{
			ID3D12CommandAllocator* allocator;
			DX_CHECK(device->GetDevice()->CreateCommandAllocator(D3DCmdListType(type), IID_PPV_ARGS(&allocator)));
			m_AllocatorPool.push({ allocator, 0ull });
		}
	}

	CommandQueue::~CommandQueue()
	{
		WaitForIdle();

		while(!m_AllocatorPool.empty())
		{
			m_AllocatorPool.front().first->Release();
			m_AllocatorPool.pop();
		}

		delete m_Fence;
	}

	void CommandQueue::GetTimestampFrequency(uint64* frequency)
	{
		DX_CHECK(m_Queue->GetTimestampFrequency(frequency));
	}

	ID3D12CommandAllocator* CommandQueue::RequestAllocator()
	{
		ID3D12CommandAllocator* allocator = nullptr;
		if (!m_Fence->IsComplete(m_AllocatorPool.front().second))
		{
			LB_WARN("%s: No Command Allocator available, waiting...", CmdListTypeToStr(m_Type).data());
			m_Fence->CpuWait(m_AllocatorPool.front().second);
		}

		allocator = m_AllocatorPool.front().first;
		m_AllocatorPool.pop();
		allocator->Reset();
		return allocator;
	}

	void CommandQueue::FreeAllocator(ID3D12CommandAllocator* allocator, uint64 fenceValue)
	{
		m_AllocatorPool.push({ allocator, fenceValue });
	}

	uint64 CommandQueue::ExecuteCommandLists(CommandContext* context)
	{
		ID3D12CommandList* cmd[1] = { context->Get() };
		m_Queue->ExecuteCommandLists(1, cmd);

		uint64 fenceValue = m_Fence->Signal(this);
		context->Free(fenceValue);
		return fenceValue;
	}

	void CommandQueue::WaitForIdle()
	{
		uint64 fenceValue = m_Fence->Signal(this);
		m_Fence->CpuWait(fenceValue);
	}
}
