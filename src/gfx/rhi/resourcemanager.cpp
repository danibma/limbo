﻿#include "stdafx.h"
#include "resourcemanager.h"

#include "gfx/gfx.h"

namespace limbo::Gfx
{
#define DELETE_RESOURCE(ResourceHandle, ResourceList) \
	Deletion deletion = {}; \
	deletion.Delegate.BindLambda([this, ResourceHandle]() \
		{ \
			ResourceList.DeleteHandle(ResourceHandle); \
		}); \
	deletion.DeletionCounter = 0; \
	m_DeletionQueue.push_back(std::move(deletion))

	ResourceManager::ResourceManager()
	{
		OnPostResourceManagerInit.AddLambda([&]()
		{
			uint32_t textureData = 0x00FFFFFF;
			EmptyTexture = CreateTexture({
				.Width = 1,
				.Height = 1,
				.DebugName = "Empty Texture",
				.Format = Format::RGBA8_UNORM,
				.Type = TextureType::Texture2D,
				.InitialData = &textureData,
				.bCreateSrv = true
			});

			DefaultLinearWrap = Gfx::CreateSampler({
				.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				.MipLODBias = 0,
				.MaxAnisotropy = 0,
				.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
				.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
				.MinLOD = 0.0f,
				.MaxLOD = D3D12_FLOAT32_MAX
			});

			DefaultLinearClamp = Gfx::CreateSampler({
				.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.MipLODBias = 0,
				.MaxAnisotropy = 0,
				.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
				.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
				.MinLOD = 0.0f,
				.MaxLOD = D3D12_FLOAT32_MAX
			});

			DefaultPointClamp = Gfx::CreateSampler({
				.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
				.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.MipLODBias = 0,
				.MaxAnisotropy = 0,
				.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
				.BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f },
				.MinLOD = 0.0f,
				.MaxLOD = D3D12_FLOAT32_MAX
			});
		});
	}

	ResourceManager::~ResourceManager()
	{
		m_bOnShutdown = true;
		DestroyTexture(EmptyTexture);

		ForceDeletionQueue();

#if !NO_LOG
		ensure(m_Buffers.IsEmpty());
		ensure(m_Textures.IsEmpty());
		ensure(m_Shaders.IsEmpty());
#endif
	}

	Handle<Buffer> ResourceManager::CreateBuffer(const BufferSpec& spec)
	{
		return m_Buffers.AllocateHandle(spec);
	}

	Handle<Shader> ResourceManager::CreateShader(const ShaderSpec& spec)
	{
		return m_Shaders.AllocateHandle(spec);
	}

	Handle<Texture> ResourceManager::CreateTextureFromFile(const char* path, const char* debugName)
	{
		return m_Textures.AllocateHandle(path, debugName);
	}

	Handle<Texture> ResourceManager::CreateTexture(const TextureSpec& spec)
	{
		return m_Textures.AllocateHandle(spec);
	}

	Handle<Texture> ResourceManager::CreateTexture(ID3D12Resource* resource, const TextureSpec& spec)
	{
		return m_Textures.AllocateHandle(resource, spec);
	}

	Handle<Sampler> ResourceManager::CreateSampler(const D3D12_SAMPLER_DESC& spec)
	{
		return m_Samplers.AllocateHandle(spec);
	}

	Gfx::Buffer* ResourceManager::GetBuffer(Handle<Buffer> buffer)
	{
		return m_Buffers.Get(buffer);
	}

	Gfx::Shader* ResourceManager::GetShader(Handle<Shader> shader)
	{
		return m_Shaders.Get(shader);
	}

	Gfx::Texture* ResourceManager::GetTexture(Handle<Texture> texture)
	{
		return m_Textures.Get(texture);
	}

	Sampler* ResourceManager::GetSampler(Handle<Sampler> sampler)
	{
		return m_Samplers.Get(sampler);
	}

	uint64 ResourceManager::GetTextureID(Handle<Texture> texture)
	{
		Texture* t = ResourceManager::Ptr->GetTexture(texture);
		return t->SRVHandle->GPUHandle.ptr;
	}

	void ResourceManager::DestroyBuffer(Handle<Buffer> buffer, bool bImmediate)
	{
		if (bImmediate)
		{
			m_Buffers.DeleteHandle(buffer);
		}
		else
		{
			DELETE_RESOURCE(buffer, m_Buffers);
		}
	}

	void ResourceManager::DestroyShader(Handle<Shader> shader, bool bImmediate)
	{
		if (bImmediate)
		{
			m_Shaders.DeleteHandle(shader);
		}
		else
		{
			DELETE_RESOURCE(shader, m_Shaders);
		}
	}

	void ResourceManager::DestroyTexture(Handle<Texture> texture, bool bImmediate)
	{
		if (bImmediate)
		{
			m_Textures.DeleteHandle(texture);
		}
		else
		{
			if (texture != EmptyTexture || m_bOnShutdown)
			{
				DELETE_RESOURCE(texture, m_Textures);
			}
		}
	}

	void ResourceManager::DestroySampler(Handle<Sampler> sampler, bool bImmediate)
	{
		if (bImmediate)
		{
			m_Samplers.DeleteHandle(sampler);
		}
		else
		{
			DELETE_RESOURCE(sampler, m_Samplers);
		}
	}

	void ResourceManager::RunDeletionQueue()
	{
		for (uint32 i = 0; i < m_DeletionQueue.size();)
		{
			if (++m_DeletionQueue[i].DeletionCounter >= NUM_BACK_BUFFERS)
				m_DeletionQueue.pop_front();
			else
				++i;
		}
	}

	void ResourceManager::ForceDeletionQueue()
	{
		while (!m_DeletionQueue.empty())
		{
			++m_DeletionQueue.front().DeletionCounter;
			m_DeletionQueue.pop_front();
		}
	}
}
