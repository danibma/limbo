#pragma once

#include "resourcemanager.h"
#include "vulkanbindgroup.h"
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
		virtual Handle<BindGroup> createBindGroup(const BindGroupSpec& spec) override;

		rhi::VulkanBuffer* getBuffer(Handle<Buffer> buffer);
		rhi::VulkanShader* getShader(Handle<Shader> shader);
		rhi::VulkanTexture* getTexture(Handle<Texture> texture);
		rhi::VulkanBindGroup* getBindGroup(Handle<BindGroup> bindGroup);

		virtual void destroyBuffer(Handle<Buffer> buffer);
		virtual void destroyShader(Handle<Shader> shader);
		virtual void destroyTexture(Handle<Texture> texture);
		virtual void destroyBindGroup(Handle<BindGroup> bindGroup) override;

	private:
		Pool<rhi::VulkanBuffer, Buffer> m_buffers;
		Pool<rhi::VulkanBindGroup, BindGroup> m_bindGroups;
		Pool<rhi::VulkanShader, Shader> m_shaders;
		Pool<rhi::VulkanTexture, Texture> m_textures;
	};
}
