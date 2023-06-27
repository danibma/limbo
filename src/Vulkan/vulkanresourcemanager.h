#pragma once

#include "resourcemanager.h"
#include "vulkanbuffer.h"
#include "vulkanshader.h"
#include "vulkantexture.h"

namespace limbo
{
	class VulkanResourceManager final : public ResourceManager
	{
	public:
		VulkanResourceManager() = default;
		virtual ~VulkanResourceManager();

		virtual Handle<Buffer> createBuffer(const BufferSpec& spec);
		virtual Handle<Shader> createShader(const ShaderSpec& spec);
		virtual Handle<Texture> createTexture(const TextureSpec& spec);

		virtual void destroyBuffer(Handle<Buffer> buffer);
		virtual void destroyShader(Handle<Shader> shader);
		virtual void destroyTexture(Handle<Texture> texture);

	private:
		Pool<VulkanBuffer, Buffer> m_buffers;
		Pool<VulkanShader, Shader> m_shaders;
		Pool<VulkanTexture, Texture> m_textures;
	};
}
