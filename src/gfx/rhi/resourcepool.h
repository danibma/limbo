#pragma once

#include "core/core.h"

#include <vector>
#include <queue>
#include <type_traits>

namespace limbo::Gfx
{
	template<typename T>
	class Handle
	{
	public:
		Handle() : m_Index(0xFFFF), m_Generation(0) {}
		bool IsValid() { return m_Index != 0xFFFF; }

		bool operator!=(const Handle& other)
		{
			return !(m_Index == other.m_Index && m_Generation == other.m_Generation);
		}

	private:
		Handle(uint16 index, uint16 generation) : m_Index(index), m_Generation(generation) {}

		uint16 m_Index;
		uint16 m_Generation;

		template<typename T1, size_t MaxSize> friend class Pool;
	};

	template<typename HandleType, size_t MaxSize>
	class Pool
	{
	public:
		Pool()
		{
			m_Objects.resize(MAX_SIZE);
			for (uint16 i = 0; i < MAX_SIZE; ++i)
			{
				m_FreeSlots.push(i);
				m_Objects[i].Data = new HandleType();
			}
		}

		template<class... Args>
		Handle<HandleType> AllocateHandle(Args&&... args)
		{
			ensure(m_FreeSlots.size() > 0);
			uint16 freeSlot = m_FreeSlots.front();
			m_FreeSlots.pop();
			ensure(freeSlot < MAX_SIZE);
			Object& obj = m_Objects[freeSlot];
			new(obj.Data) HandleType(std::forward<Args>(args)...);
			return Handle<HandleType>(freeSlot, obj.Generation);
		}

		void DeleteHandle(Handle<HandleType> handle)
		{
			m_FreeSlots.push(handle.m_Index);
			ensure(m_FreeSlots.size() <= MAX_SIZE);
			ensure(handle.IsValid());
			m_Objects[handle.m_Index].Data->~HandleType();
			++m_Objects[handle.m_Index].Generation;
		}

		HandleType* Get(Handle<HandleType> handle)
		{
			ensure(handle.IsValid());
			const Object& obj = m_Objects[handle.m_Index];
			if (handle.m_Generation != obj.Generation)
				return nullptr;
			return obj.Data;
		}

		uint16 GetSize()
		{
			return MAX_SIZE;
		}

		bool IsEmpty()
		{
			return m_Objects.size() == m_FreeSlots.size();
		}

	private:
		struct Object
		{
			HandleType* Data;
			uint16		Generation;
		};

		std::vector<Object> m_Objects;
		std::queue<uint16> m_FreeSlots;

		const uint16 MAX_SIZE = MaxSize;
	};
}