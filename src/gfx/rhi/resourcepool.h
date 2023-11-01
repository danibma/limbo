#pragma once

#include "core/core.h"

#include <queue>

namespace limbo::RHI
{
	template<typename T>
	class Handle
	{
	public:
		Handle() : m_Index(0xFFFF), m_Generation(0) {}
		bool IsValid() const { return m_Index != 0xFFFF; }

		bool operator!=(const Handle& other)
		{
			return !(m_Index == other.m_Index && m_Generation == other.m_Generation);
		}

		bool operator==(const Handle& other)
		{
			return m_Index == other.m_Index && m_Generation == other.m_Generation;
		}

	private:
		Handle(uint16 index, uint16 generation) : m_Index(index), m_Generation(generation) {}

		uint16 m_Index;
		uint16 m_Generation;

		template<typename T1, uint16 InitialSize> friend class Pool;
	};

	template<typename HandleType, uint16 InitialSize>
	class Pool
	{
	public:
		Pool()
		{
			m_Objects = (Object*)malloc(m_Size * sizeof(Object));
			memset(m_Objects, 0, m_Size * sizeof(Object));
			for (uint16 i = 0; i < m_Size; ++i)
			{
				m_FreeSlots.push(i);
				m_Objects[i].Data = new HandleType();
			}
		}

		~Pool()
		{
			for (uint16 i = 0; i < m_Size; ++i)
				free(m_Objects[i].Data);
			free(m_Objects);
		}

		template<class... Args>
		Handle<HandleType> AllocateHandle(Args&&... args)
		{
			if (m_FreeSlots.size() == 0) Grow();
			uint16 freeSlot = m_FreeSlots.front();
			m_FreeSlots.pop();
			ensure(freeSlot < m_Size);
			Object& obj = m_Objects[freeSlot];
			new(obj.Data) HandleType(std::forward<Args>(args)...);
			return Handle<HandleType>(freeSlot, obj.Generation);
		}

		void DeleteHandle(Handle<HandleType> handle)
		{
			ensure(handle.IsValid());
			m_FreeSlots.push(handle.m_Index);
			ensure(m_FreeSlots.size() <= m_Size);
			m_Objects[handle.m_Index].Data->~HandleType();
			++m_Objects[handle.m_Index].Generation;
		}

		void Grow()
		{
			uint16 newSize = m_Size + (m_Size >> 1);
			LB_WARN("Growing '%s' resource pool from %d to %d", typeid(HandleType).name(), m_Size, newSize);
			Object* newObjects = (Object*)malloc(newSize * sizeof(Object));
			memset(newObjects, 0, newSize * sizeof(Object));
			memcpy(newObjects, m_Objects, m_Size * sizeof(Object));
			for (uint16 i = m_Size; i < newSize; ++i)
			{
				m_FreeSlots.push(i);
				newObjects[i].Data = new HandleType();
			}
			free(m_Objects);
			m_Objects = newObjects;
			m_Size = newSize;
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
			return m_Size;
		}

		bool IsEmpty()
		{
			return m_Size == (uint16)m_FreeSlots.size();
		}

	private:
		struct Object
		{
			HandleType* Data;
			uint16		Generation;
		};

		Object*			   m_Objects;
		std::queue<uint16> m_FreeSlots;

		uint16 m_Size = InitialSize;
	};
}