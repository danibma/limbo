#pragma once

#include "resourcemanager.h"
#include "d3d12bindgroup.h"
#include "d3d12buffer.h"
#include "d3d12shader.h"
#include "d3d12texture.h"

namespace limbo::rhi
{
	class D3D12ResourceManager final : public ResourceManager
	{
	public:
		D3D12ResourceManager() = default;
		virtual ~D3D12ResourceManager();

		Handle<Buffer> createBuffer(const BufferSpec& spec) override;
		Handle<Shader> createShader(const ShaderSpec& spec) override;
		Handle<Texture> createTexture(const TextureSpec& spec) override;
		Handle<BindGroup> createBindGroup(const BindGroupSpec& spec) override;

		rhi::D3D12Buffer* getBuffer(Handle<Buffer> buffer);
		rhi::D3D12Shader* getShader(Handle<Shader> shader);
		rhi::D3D12Texture* getTexture(Handle<Texture> texture);
		rhi::D3D12BindGroup* getBindGroup(Handle<BindGroup> bindGroup);

		void destroyBuffer(Handle<Buffer> buffer) override;
		void destroyShader(Handle<Shader> shader) override;
		void destroyTexture(Handle<Texture> texture) override;
		void destroyBindGroup(Handle<BindGroup> bindGroup) override;

	private:
		Pool<D3D12Buffer, Buffer> m_buffers;
		Pool<D3D12Shader, Shader> m_shaders;
		Pool<D3D12Texture, Texture> m_textures;
		Pool<D3D12BindGroup, BindGroup> m_bindGroups;
	};
}
