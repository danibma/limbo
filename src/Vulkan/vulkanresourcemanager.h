#pragma once

#include "resourcemanager.h"
#include "vulkanbindgroup.h"
#include "vulkanbuffer.h"
#include "vulkanshader.h"
#include "vulkantexture.h"

namespace limbo::rhi
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

		VulkanBuffer* getBuffer(Handle<Buffer> buffer);
		VulkanShader* getShader(Handle<Shader> shader);
		VulkanTexture* getTexture(Handle<Texture> texture);
		VulkanBindGroup* getBindGroup(Handle<BindGroup> bindGroup);

		virtual void destroyBuffer(Handle<Buffer> buffer);
		virtual void destroyShader(Handle<Shader> shader);
		virtual void destroyTexture(Handle<Texture> texture);
		virtual void destroyBindGroup(Handle<BindGroup> bindGroup) override;

	private:
		Pool<VulkanBuffer, Buffer> m_buffers;
		Pool<VulkanBindGroup, BindGroup> m_bindGroups;
		Pool<VulkanShader, Shader> m_shaders;
		Pool<VulkanTexture, Texture> m_textures;
	};
}
