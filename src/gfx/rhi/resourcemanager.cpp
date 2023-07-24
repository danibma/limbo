#include "resourcemanager.h"

namespace limbo::gfx
{
	ResourceManager::~ResourceManager()
	{
#if LIMBO_DEBUG
		ensure(m_buffers.isEmpty());
		ensure(m_textures.isEmpty());
		ensure(m_shaders.isEmpty());
#endif
	}

	Handle<Buffer> ResourceManager::createBuffer(const BufferSpec& spec)
	{
		return m_buffers.allocateHandle(spec);
	}

	Handle<Shader> ResourceManager::createShader(const ShaderSpec& spec)
	{
		return m_shaders.allocateHandle(spec);
	}

	Handle<Texture> ResourceManager::createTexture(const TextureSpec& spec)
	{
		return m_textures.allocateHandle(spec);
	}

	Handle<Texture> ResourceManager::createTexture(ID3D12Resource* resource, const TextureSpec& spec)
	{
		return m_textures.allocateHandle(resource, spec);
	}

	Handle<Sampler> ResourceManager::createSampler(const D3D12_SAMPLER_DESC& spec)
	{
		return m_samplers.allocateHandle(spec);
	}

	gfx::Buffer* ResourceManager::getBuffer(Handle<Buffer> buffer)
	{
		return m_buffers.get(buffer);
	}

	gfx::Shader* ResourceManager::getShader(Handle<Shader> shader)
	{
		return m_shaders.get(shader);
	}

	gfx::Texture* ResourceManager::getTexture(Handle<Texture> texture)
	{
		return m_textures.get(texture);
	}

	Sampler* ResourceManager::getSampler(Handle<Sampler> sampler)
	{
		return m_samplers.get(sampler);
	}

	void ResourceManager::destroyBuffer(Handle<Buffer> buffer)
	{
		m_buffers.deleteHandle(buffer);
	}

	void ResourceManager::destroyShader(Handle<Shader> shader)
	{
		m_shaders.deleteHandle(shader);
	}

	void ResourceManager::destroyTexture(Handle<Texture> texture)
	{
		m_textures.deleteHandle(texture);
	}

	void ResourceManager::destroySampler(Handle<Sampler> sampler)
	{
		m_samplers.deleteHandle(sampler);
	}
}
