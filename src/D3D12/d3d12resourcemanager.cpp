#include "d3d12resourcemanager.h"

namespace limbo::rhi
{
	D3D12ResourceManager::~D3D12ResourceManager()
	{
#if LIMBO_DEBUG
		ensure(m_buffers.isEmpty());
		ensure(m_textures.isEmpty());
		ensure(m_shaders.isEmpty());
		ensure(m_bindGroups.isEmpty());
#endif
	}

	Handle<Buffer> D3D12ResourceManager::createBuffer(const BufferSpec& spec)
	{
		return m_buffers.allocateHandle(spec);
	}

	Handle<Shader> D3D12ResourceManager::createShader(const ShaderSpec& spec)
	{
		return m_shaders.allocateHandle(spec);
	}

	Handle<Texture> D3D12ResourceManager::createTexture(const TextureSpec& spec)
	{
		return m_textures.allocateHandle(spec);
	}

	Handle<BindGroup> D3D12ResourceManager::createBindGroup(const BindGroupSpec& spec)
	{
		return m_bindGroups.allocateHandle(spec);
	}

	rhi::D3D12Buffer* D3D12ResourceManager::getBuffer(Handle<Buffer> buffer)
	{
		return m_buffers.get(buffer);
	}

	rhi::D3D12Shader* D3D12ResourceManager::getShader(Handle<Shader> shader)
	{
		return m_shaders.get(shader);
	}

	rhi::D3D12Texture* D3D12ResourceManager::getTexture(Handle<Texture> texture)
	{
		return m_textures.get(texture);
	}

	rhi::D3D12BindGroup* D3D12ResourceManager::getBindGroup(Handle<BindGroup> bindGroup)
	{
		return m_bindGroups.get(bindGroup);
	}

	void D3D12ResourceManager::destroyBuffer(Handle<Buffer> buffer)
	{
		m_buffers.deleteHandle(buffer);
	}

	void D3D12ResourceManager::destroyShader(Handle<Shader> shader)
	{
		m_shaders.deleteHandle(shader);
	}

	void D3D12ResourceManager::destroyTexture(Handle<Texture> texture)
	{
		m_textures.deleteHandle(texture);
	}

	void D3D12ResourceManager::destroyBindGroup(Handle<BindGroup> bindGroup)
	{
		m_bindGroups.deleteHandle(bindGroup);
	}
}
