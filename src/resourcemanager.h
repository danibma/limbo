#pragma once

#include "resourcepool.h"

namespace limbo
{
	class Buffer;
	class Shader;
	class Texture;
	class BindGroup;
	struct BufferSpec;
	struct ShaderSpec;
	struct TextureSpec;
	struct BindGroupSpec;
	class ResourceManager
	{
	public:
		static ResourceManager* ptr;

		template<typename T>
		[[nodiscard]] static T* getAs()
		{
			static_assert(std::is_base_of_v<ResourceManager, T>);
			ensure(ptr);
			return static_cast<T*>(ptr);
		}

		virtual ~ResourceManager() = default;

		virtual Handle<Buffer> createBuffer(const BufferSpec& spec) = 0;
		virtual Handle<Shader> createShader(const ShaderSpec& spec) = 0;
		virtual Handle<Texture> createTexture(const TextureSpec& spec) = 0;
		virtual Handle<BindGroup> createBindGroup(const BindGroupSpec& spec) = 0;

		virtual void destroyBuffer(Handle<Buffer> buffer) = 0;
		virtual void destroyShader(Handle<Shader> shader) = 0;
		virtual void destroyTexture(Handle<Texture> texture) = 0;
		virtual void destroyBindGroup(Handle<BindGroup> bindGroup) = 0;
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

	inline Handle<BindGroup> createBindGroup(const BindGroupSpec& spec)
	{
		return ResourceManager::ptr->createBindGroup(spec);
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

	inline void destroyBindGroup(Handle<BindGroup> handle)
	{
		ResourceManager::ptr->destroyBindGroup(handle);
	}
}