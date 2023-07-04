#pragma once

#include "resourcemanager.h"
#include "core.h"
#include "draw.h"

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

		virtual void copyTextureToBackBuffer(Handle<Texture> texture) = 0;

		virtual void bindDrawState(const DrawInfo& drawState) = 0;
		virtual void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 1, uint32 firstInstance = 1) = 0;

		virtual void dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) = 0;

		virtual void present() = 0;
	};

	inline void copyTextureToBackBuffer(Handle<Texture> texture)
	{
		Device::ptr->copyTextureToBackBuffer(texture);
	}

	inline void bindDrawState(const DrawInfo&& drawState)
	{
		Device::ptr->bindDrawState(drawState);
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
