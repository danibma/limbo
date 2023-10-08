#include "stdafx.h"
#include "resourcemanager.h"

#include "device.h"
#include "gfx/gfx.h"

namespace limbo::RHI
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
		Gfx::OnPostResourceManagerInit.AddLambda([&]()
		{
			uint32_t textureData = 0x00FFFFFF;
			EmptyTexture = CreateTexture({
				.Width = 1,
				.Height = 1,
				.DebugName = "Empty Texture",
				.Flags = TextureUsage::ShaderResource,
				.Format = Format::RGBA8_UNORM,
				.Type = TextureType::Texture2D,
				.InitialData = &textureData,
			});
		});

		Device::Ptr->OnPrepareFrame.AddLambda([&]()
		{
			RunDeletionQueue();
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
		ensure(m_PSOs.IsEmpty());
		ensure(m_RootSignatures.IsEmpty());
		ensure(m_Shaders.IsEmpty());
#endif
	}

	BufferHandle ResourceManager::CreateBuffer(const BufferSpec& spec)
	{
		return m_Buffers.AllocateHandle(spec);
	}

	ShaderHandle ResourceManager::CreateShader(const char* file, const char* entryPoint, ShaderType type)
	{
		return m_Shaders.AllocateHandle(file, entryPoint, type);
	}

	TextureHandle ResourceManager::CreateTextureFromFile(const char* path, const char* debugName)
	{
		return m_Textures.AllocateHandle(path, debugName);
	}

	TextureHandle ResourceManager::CreateTexture(const TextureSpec& spec)
	{
		return m_Textures.AllocateHandle(spec);
	}

	TextureHandle ResourceManager::CreateTexture(ID3D12Resource* resource, const TextureSpec& spec)
	{
		return m_Textures.AllocateHandle(resource, spec);
	}

	PSOHandle ResourceManager::CreatePSO(const PipelineStateInitializer& initializer)
	{
		return m_PSOs.AllocateHandle(initializer);
	}

	PSOHandle ResourceManager::CreatePSO(const RaytracingPipelineStateInitializer& initializer)
	{
		return m_PSOs.AllocateHandle(initializer);
	}

	RootSignatureHandle ResourceManager::CreateRootSignature(const std::string& name, const RSInitializer& initializer)
	{
		return m_RootSignatures.AllocateHandle(name, initializer);
	}

	void ResourceManager::DestroyBuffer(BufferHandle buffer, bool bImmediate)
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

	void ResourceManager::DestroyShader(ShaderHandle shader, bool bImmediate)
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

	void ResourceManager::DestroyTexture(TextureHandle texture, bool bImmediate)
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

	void ResourceManager::DestroyPSO(PSOHandle pso, bool bImmediate /*= false*/)
	{
		if (bImmediate)
		{
			m_PSOs.DeleteHandle(pso);
		}
		else
		{
			DELETE_RESOURCE(pso, m_PSOs);
		}
	}

	void ResourceManager::DestroyRootSignature(RootSignatureHandle rs, bool bImmediate /*= false*/)
	{
		if (bImmediate)
		{
			m_RootSignatures.DeleteHandle(rs);
		}
		else
		{
			DELETE_RESOURCE(rs, m_RootSignatures);
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
