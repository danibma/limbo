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
			{
				m_freeSlots.push(i);
				m_objects[i].data = new DataType();
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
			new(obj.data) DataType(std::forward<Args>(args)...);
			return Handle<HandleType>(freeSlot, obj.generation);
		}

		void deleteHandle(Handle<HandleType> handle)
		{
			m_freeSlots.push(handle.m_index);
			ensure(handle.isValid());
			m_objects[handle.m_index].data->~DataType();
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

		uint16 getSize()
		{
			return MAX_SIZE;
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

	class Buffer;
	class Shader;
	class Texture;
	struct BufferSpec;
	struct ShaderSpec;
	struct TextureSpec;
	class ResourceManager
	{
	public:
		static ResourceManager* ptr;

		virtual ~ResourceManager() = default;

		virtual Handle<Buffer> createBuffer(const BufferSpec& spec) = 0;
		virtual Handle<Shader> createShader(const ShaderSpec& spec) = 0;
		virtual Handle<Texture> createTexture(const TextureSpec& spec) = 0;

		virtual void destroyBuffer(Handle<Buffer> buffer) = 0;
		virtual void destroyShader(Handle<Shader> shader) = 0;
		virtual void destroyTexture(Handle<Texture> texture) = 0;
	};

	// Global definitions
	inline Handle<Buffer> createBuffer(const BufferSpec& spec)
	{
		return ResourceManager::ptr->createBuffer(spec);
	}

	inline Handle<Shader> createShader(const ShaderSpec& spec)
	{
		return ResourceManager::ptr->createShader(spec);
	}

	inline Handle<Texture> createTexture(const TextureSpec& spec)
	{
		return ResourceManager::ptr->createTexture(spec);
	}

	inline void destroyBuffer(Handle<Buffer> handle)
	{
		ResourceManager::ptr->destroyBuffer(handle);
	}

	inline void destroyShader(Handle<Shader> handle)
	{
		ResourceManager::ptr->destroyShader(handle);
	}

	inline void destroyTexture(Handle<Texture> handle)
	{
		ResourceManager::ptr->destroyTexture(handle);
	}
}