#pragma once

#include "resourcemanager.h"
#include "core.h"

namespace limbo
{
	class Device
	{
	public:
		static Device* ptr;

		virtual ~Device() = default;

		virtual void setParameter(Handle<Shader> shader, uint8 slot, const void* data) = 0;
		virtual void setParameter(Handle<Shader> shader, uint8 slot, Handle<Buffer> buffer) = 0;
		virtual void setParameter(Handle<Shader> shader, uint8 slot, Handle<Texture> texture) = 0;

		virtual void bindShader(Handle<Shader> shader) = 0;
		virtual void bindVertexBuffer(Handle<Buffer> vertexBuffer) = 0;
		virtual void bindIndexBuffer(Handle<Buffer> indexBuffer) = 0;

		virtual void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 1, uint32 firstInstance = 1) = 0;

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

	inline void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 1, uint32 firstInstance = 1)
	{
		Device::ptr->draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	inline void present()
	{
		Device::ptr->present();
	}
}
