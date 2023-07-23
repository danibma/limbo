#pragma once

#include "resourcemanager.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"

namespace limbo::gfx
{
	class ResourceManager
	{
	public:
		static ResourceManager* ptr;

	public:
		ResourceManager() = default;
		virtual ~ResourceManager();

		Handle<Buffer> createBuffer(const BufferSpec& spec);
		Handle<Shader> createShader(const ShaderSpec& spec);
		Handle<Texture> createTexture(const TextureSpec& spec);
		Handle<Texture> createTexture(ID3D12Resource* resource, const TextureSpec& spec);

		Buffer* getBuffer(Handle<Buffer> buffer);
		Shader* getShader(Handle<Shader> shader);
		Texture* getTexture(Handle<Texture> texture);

		void destroyBuffer(Handle<Buffer> buffer);
		void destroyShader(Handle<Shader> shader);
		void destroyTexture(Handle<Texture> texture);

	private:
		Pool<Buffer> m_buffers;
		Pool<Shader> m_shaders;
		Pool<Texture> m_textures;
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

	inline Handle<Texture> createTexture(ID3D12Resource* resource, const TextureSpec& spec)
	{
		return ResourceManager::ptr->createTexture(resource, spec);
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

	inline void setParameter(Handle<Shader> shader, const char* parameterName, Handle<Texture> texture)
	{
		Shader* s = ResourceManager::ptr->getShader(shader);
		FAILIF(!s);
		s->setTexture(parameterName, texture);
	}

	inline void setParameter(Handle<Shader> shader, const char* parameterName, Handle<Buffer> buffer)
	{
		Shader* s = ResourceManager::ptr->getShader(shader);
		FAILIF(!s);
		s->setBuffer(parameterName, buffer);
	}

	template<typename T>
	inline void setParameter(Handle<Shader> shader, const char* parameterName, const T& value)
	{
		Shader* s = ResourceManager::ptr->getShader(shader);
		FAILIF(!s);
		s->setConstant(parameterName, &value);
	}
}
