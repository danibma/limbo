#pragma once

#include "core.h"

#include <vector>
#include <queue>
#include <type_traits>

namespace limbo
{
	template<typename T>
	class Handle
	{
	public:
		Handle() : m_index(0), m_generation(0) {}
		bool isValid() { return m_index != ~0; }

	private:
		Handle(uint16 index, uint16 generation) : m_index(index), m_generation(generation) {}

		uint16 m_index;
		uint16 m_generation;

		template<typename T1, typename T2> friend class Pool;
	};

	template<typename DataType, typename HandleType> 
	class Pool
	{
	public:
		Pool()
		{
			static_assert(std::is_base_of<HandleType, DataType>::value, "DataType has to inherit from HandleType!");

			m_objects.resize(MAX_SIZE);
			for (uint16 i = 0; i < MAX_SIZE; ++i)
				m_freeSlots.push(i);
		}

		Handle<HandleType> allocateHandle(DataType* newData)
		{
			uint16 freeSlot = m_freeSlots.front();
			m_freeSlots.pop();
			ensure(freeSlot < MAX_SIZE);
			ensure(freeSlot != ~0);
			Object& obj = m_objects[freeSlot];
			obj.data = newData;
			return Handle<HandleType>(freeSlot, obj.generation);
		}

		void deleteHandle(Handle<HandleType> handle)
		{
			m_freeSlots.push(handle.m_index);
			ensure(handle.isValid());
			m_objects[handle.m_index].data = nullptr;
			m_objects[handle.m_index].generation++;
		}

		DataType* get(Handle<HandleType> handle)
		{
			ensure(handle.isValid());
			const Object& obj = m_objects[handle.m_index];
			if (handle.m_generation != obj.generation)
				return nullptr;
			return obj.data;
		}

	private:
		struct Object
		{
			DataType* data;
			uint16 generation;
		};

		std::vector<Object> m_objects;
		std::queue<uint16> m_freeSlots;

		const uint16 MAX_SIZE = 32;
	};

	class ResourceManager
	{
	public:
		static ResourceManager* ptr;

		virtual ~ResourceManager() = default;
	};
}