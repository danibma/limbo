#include "vulkanresourcemanager.h"

namespace limbo
{
	VulkanResourceManager::~VulkanResourceManager()
	{
#if LIMBO_DEBUG
		ensure(m_buffers.isEmpty());
		ensure(m_textures.isEmpty());
		ensure(m_shaders.isEmpty());
#endif
	}

	Handle<Buffer> VulkanResourceManager::createBuffer(const BufferSpec& spec)
	{
		return m_buffers.allocateHandle(spec);
	}

	Handle<Shader> VulkanResourceManager::createShader(const ShaderSpec& spec)
	{
		return m_shaders.allocateHandle(spec);
	}

	Handle<Texture> VulkanResourceManager::createTexture(const TextureSpec& spec)
	{
		return m_textures.allocateHandle(spec);
	}

	void VulkanResourceManager::destroyBuffer(Handle<Buffer> buffer)
	{
		m_buffers.deleteHandle(buffer);
	}

	void VulkanResourceManager::destroyShader(Handle<Shader> shader)
	{
		m_shaders.deleteHandle(shader);
	}

	void VulkanResourceManager::destroyTexture(Handle<Texture> texture)
	{
		m_textures.deleteHandle(texture);
	}

}
