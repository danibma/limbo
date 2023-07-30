#include "stdafx.h"
#include "resourcemanager.h"
#include "gfx/gfx.h"

namespace limbo::gfx
{
#define DELETE_RESOURCE(ResourceHandle, ResourceList) \
	Deletion deletion = {}; \
	deletion.delegate.BindLambda([this, ResourceHandle]() \
		{ \
			ResourceList.deleteHandle(ResourceHandle); \
		}); \
	deletion.deletionCounter = 0; \
	m_deletionQueue.push_back(std::move(deletion))

	ResourceManager::ResourceManager()
	{
		onPostResourceManagerInit.AddLambda([&]()
		{
			uint32_t blackTextureData = 0x00000000;
			emptyTexture = createTexture({
				.width = 1,
				.height = 1,
				.debugName = "Empty Texture",
				.format = Format::RGBA8_UNORM,
				.type = TextureType::Texture2D,
				.initialData = &blackTextureData,
				.bCreateSrv = true
			});
		});
	}

	ResourceManager::~ResourceManager()
	{
		m_onShutdown = true;
		destroyTexture(emptyTexture);

		forceDeletionQueue();

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
		DELETE_RESOURCE(buffer, m_buffers);
	}

	void ResourceManager::destroyShader(Handle<Shader> shader)
	{
		DELETE_RESOURCE(shader, m_shaders);;
	}

	void ResourceManager::destroyTexture(Handle<Texture> texture)
	{
		if (texture != emptyTexture || m_onShutdown)
		{
			DELETE_RESOURCE(texture, m_textures);
		}
	}

	void ResourceManager::destroySampler(Handle<Sampler> sampler)
	{
		DELETE_RESOURCE(sampler, m_samplers);
	}

	void ResourceManager::runDeletionQueue()
	{
		for (uint32 i = 0; i < m_deletionQueue.size();)
		{
			if (++m_deletionQueue[i].deletionCounter >= NUM_BACK_BUFFERS)
				m_deletionQueue.pop_front();
			else
				++i;
		}
	}

	void ResourceManager::forceDeletionQueue()
	{
		while (!m_deletionQueue.empty())
		{
			++m_deletionQueue.front().deletionCounter;
			m_deletionQueue.pop_front();
		}
	}
}
