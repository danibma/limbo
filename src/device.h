#pragma once

#include "resourcemanager.h"
#include "core.h"

#include <type_traits>

namespace limbo
{
	class Device
	{
	public:
		static Device* ptr;

		template<typename T>
		[[nodiscard]] static T* getAs()
		{
			static_assert(std::is_base_of_v<Device, T>);
			ensure(ptr);
			return static_cast<T*>(ptr);
		}

		virtual ~Device() = default;

		virtual void setParameter(Handle<Shader> shader, uint8 slot, const void* data) = 0;
		virtual void setParameter(Handle<Shader> shader, uint8 slot, Handle<Buffer> buffer) = 0;
		virtual void setParameter(Handle<Shader> shader, uint8 slot, Handle<Texture> texture) = 0;

		virtual void bindShader(Handle<Shader> shader) = 0;
		virtual void bindVertexBuffer(Handle<Buffer> vertexBuffer) = 0;
		virtual void bindIndexBuffer(Handle<Buffer> indexBuffer) = 0;

		virtual void copyTextureToBackBuffer(Handle<Texture> texture) = 0;

		virtual void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 1, uint32 firstInstance = 1) = 0;

		virtual void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) = 0;

		virtual void present() = 0;
	};

	template<typename T>
	inline void setParameter(Handle<Shader> shader, uint8 slot, const T& data)
	{
		Device::ptr->setParameter(shader, slot, &data);
	}

	inline void setParameter(Handle<Shader> shader, uint8 slot, Handle<Buffer> buffer)
	{
		Device::ptr->setParameter(shader, slot, buffer);
	}

	inline void setParameter(Handle<Shader> shader, uint8 slot, Handle<Texture> texture)
	{
		Device::ptr->setParameter(shader, slot, texture);
	}

	inline void bindShader(Handle<Shader> shader)
	{
		Device::ptr->bindShader(shader);
	}

	inline void bindVertexBuffer(Handle<Buffer> vertexBuffer)
	{
		Device::ptr->bindVertexBuffer(vertexBuffer);
	}

	inline void bindIndexBuffer(Handle<Buffer> indexBuffer)
	{
		Device::ptr->bindIndexBuffer(indexBuffer);
	}

	inline void copyTextureToBackBuffer(Handle<Texture> texture)
	{
		Device::ptr->copyTextureToBackBuffer(texture);
	}

	inline void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 1, uint32 firstInstance = 1)
	{
		Device::ptr->draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	inline void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ)
	{
		Device::ptr->dispatch(groupCountX, groupCountY, groupCountZ);
	}

	inline void present()
	{
		Device::ptr->present();
	}
}
