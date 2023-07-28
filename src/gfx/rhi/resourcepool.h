#pragma once

#include "core/core.h"

#include <vector>
#include <queue>
#include <type_traits>

namespace limbo::gfx
{
	template<typename T>
	class Handle
	{
	public:
		Handle() : m_index(0xFFFF), m_generation(0) {}
		bool isValid() { return m_index != 0xFFFF; }

	private:
		Handle(uint16 index, uint16 generation) : m_index(index), m_generation(generation) {}

		uint16 m_index;
		uint16 m_generation;

		template<typename T1> friend class Pool;
	};

	template<typename HandleType>
	class Pool
	{
	public:
		Pool()
		{
			m_objects.resize(MAX_SIZE);
			for (uint16 i = 0; i < MAX_SIZE; ++i)
			{
				m_freeSlots.push(i);
				m_objects[i].data = new HandleType();
			}
		}

		template<class... Args>
		Handle<HandleType> allocateHandle(Args&&... args)
		{
			uint16 freeSlot = m_freeSlots.front();
			m_freeSlots.pop();
			ensure(freeSlot < MAX_SIZE);
			ensure(freeSlot != 0xff);
			Object& obj = m_objects[freeSlot];
			new(obj.data) HandleType(std::forward<Args>(args)...);
			return Handle<HandleType>(freeSlot, obj.generation);
		}

		void deleteHandle(Handle<HandleType> handle)
		{
			m_freeSlots.push(handle.m_index);
			ensure(m_freeSlots.size() <= MAX_SIZE);
			ensure(handle.isValid());
			m_objects[handle.m_index].data->~HandleType();
			++m_objects[handle.m_index].generation;
		}

		HandleType* get(Handle<HandleType> handle)
		{
			ensure(handle.isValid());
			const Object& obj = m_objects[handle.m_index];
			if (handle.m_generation != obj.generation)
				return nullptr;
			return obj.data;
		}

		uint16 getSize()
		{
			return MAX_SIZE;
		}

		bool isEmpty()
		{
			return m_objects.size() == m_freeSlots.size();
		}

	private:
		struct Object
		{
			HandleType* data;
			uint16 generation;
		};

		std::vector<Object> m_objects;
		std::queue<uint16> m_freeSlots;

		const uint16 MAX_SIZE = 256;
	};
}