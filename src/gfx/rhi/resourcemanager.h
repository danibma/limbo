#pragma once

#include "resourcemanager.h"
#include "bindgroup.h"
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
		Handle<BindGroup> createBindGroup(const BindGroupSpec& spec);

		Buffer* getBuffer(Handle<Buffer> buffer);
		Shader* getShader(Handle<Shader> shader);
		Texture* getTexture(Handle<Texture> texture);
		BindGroup* getBindGroup(Handle<BindGroup> bindGroup);

		void destroyBuffer(Handle<Buffer> buffer);
		void destroyShader(Handle<Shader> shader);
		void destroyTexture(Handle<Texture> texture);
		void destroyBindGroup(Handle<BindGroup> bindGroup);

	private:
		Pool<Buffer> m_buffers;
		Pool<Shader> m_shaders;
		Pool<Texture> m_textures;
		Pool<BindGroup> m_bindGroups;
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
