#pragma once

#include "core/refcountptr.h"
#include "definitions.h"

namespace limbo::RHI
{
	class CommandContext;
	class CommandQueue;
	class Device;
	class Fence
	{
		RefCountPtr<ID3D12Fence>	m_Fence;
		HANDLE				m_CompleteEvent;
		uint64				m_CurrentValue;

	public:
		explicit Fence(Device* device);
		~Fence();

		// Signals on the GPU timeline, increments the next value and returns the signaled fence value
		uint64 Signal(CommandQueue* queue);

		// Stall CPU until fence value is signaled on the GPU
		void CpuWait(uint64 fenceValue);
		void CpuWait();

		// Returns true if the fence has reached this value or higher
		bool IsComplete(uint64 fenceValue);

		// Get the fence value that will get signaled next
		uint64 GetCurrentValue() const { return m_CurrentValue; }

		ID3D12Fence* Get() const { return m_Fence.Get(); }
	};

	class CommandQueue
	{
		using CommandAllocatorPool = std::queue<std::pair<ID3D12CommandAllocator*, uint64>>;

	private:
		RefCountPtr<ID3D12CommandQueue>	m_Queue;
		ContextType					m_Type;
		Fence*						m_Fence;

		CommandAllocatorPool		m_AllocatorPool;

	public:
		CommandQueue(ContextType type, Device* device);
		~CommandQueue();

		ID3D12CommandQueue* Get() const { return m_Queue.Get(); }
		Fence* GetFence() const { return m_Fence; }

		void GetTimestampFrequency(uint64* frequency);

		ID3D12CommandAllocator* RequestAllocator();
		void FreeAllocator(ID3D12CommandAllocator* allocator, uint64 fenceValue);

		// At the moment we only have one command list of each type, if in the future we have multiple,
		// make this take a span as paremeter instead of a single context
		uint64 ExecuteCommandLists(CommandContext* context);
		void WaitForIdle();
	};
}

