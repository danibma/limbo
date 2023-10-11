#pragma once

#include "buffer.h"
#include "texture.h"
#include "rootsignature.h"
#include "pipelinestateobject.h"
#include "shader.h"

namespace limbo::RHI
{
	struct Deletion
	{
		DECLARE_DELEGATE(DestroyDelegate);
		DestroyDelegate		Delegate;
		uint8				DeletionCounter = 0;

		~Deletion()
		{
			if (!!DeletionCounter)
				Delegate.ExecuteIfBound();
		}
	};

	class ResourceManager
	{
	public:
		static ResourceManager*		Ptr;

		TextureHandle				EmptyTexture;

	public:
		ResourceManager();
		virtual ~ResourceManager();

		BufferHandle CreateBuffer(const BufferSpec& spec);
		ShaderHandle CreateShader(const char* file, const char* entryPoint, ShaderType type);
		TextureHandle CreateTextureFromFile(const char* path, const char* debugName);
		TextureHandle CreateTexture(const TextureSpec& spec);
		TextureHandle CreateTexture(ID3D12Resource* resource, const TextureSpec& spec);
		PSOHandle CreatePSO(const PipelineStateSpec& initializer);
		PSOHandle CreatePSO(const RTPipelineStateSpec& initializer);
		RootSignatureHandle CreateRootSignature(const std::string& name, const RSSpec& initializer);

		Texture* Get(TextureHandle handle) { return m_Textures.Get(handle); }
		Buffer* Get(BufferHandle handle) { return m_Buffers.Get(handle); }
		Shader* Get(ShaderHandle handle) { return m_Shaders.Get(handle); }
		RootSignature* Get(RootSignatureHandle handle) { return m_RootSignatures.Get(handle); }
		PipelineStateObject* Get(PSOHandle handle) { return m_PSOs.Get(handle); }

		void DestroyBuffer(BufferHandle buffer, bool bImmediate = false);
		void DestroyShader(ShaderHandle shader, bool bImmediate = false);
		void DestroyTexture(TextureHandle texture, bool bImmediate = false);
		void DestroyPSO(PSOHandle pso, bool bImmediate = false);
		void DestroyRootSignature(RootSignatureHandle rs, bool bImmediate = false);

		void RunDeletionQueue();
		void ForceDeletionQueue();

	private:
		Pool<Buffer,			  1<<7>		m_Buffers;
		Pool<Texture,			  1<<7>	    m_Textures;
		Pool<Shader,			   128>		m_Shaders;
		Pool<RootSignature,		    32>		m_RootSignatures;
		Pool<PipelineStateObject,   32>		m_PSOs;

		bool								m_bOnShutdown = false;

		std::deque<Deletion>				m_DeletionQueue;
	};

	// Global definitions
	FORCEINLINE [[nodiscard]] BufferHandle CreateBuffer(const BufferSpec& spec)
	{
		return ResourceManager::Ptr->CreateBuffer(spec);
	}

	FORCEINLINE [[nodiscard]] ShaderHandle CreateShader(const char* file, const char* entryPoint, ShaderType type)
	{
		return ResourceManager::Ptr->CreateShader(file, entryPoint, type);
	}

	FORCEINLINE [[nodiscard]] TextureHandle CreateTextureFromFile(const char* path, const char* debugName)
	{
		return ResourceManager::Ptr->CreateTextureFromFile(path, debugName);
	}

	FORCEINLINE [[nodiscard]] TextureHandle CreateTexture(const TextureSpec& spec)
	{
		return ResourceManager::Ptr->CreateTexture(spec);
	}

	FORCEINLINE [[nodiscard]] TextureHandle CreateTexture(ID3D12Resource* resource, const TextureSpec& spec)
	{
		return ResourceManager::Ptr->CreateTexture(resource, spec);
	}

	FORCEINLINE [[nodiscard]] PSOHandle CreatePSO(const PipelineStateSpec& initializer)
	{
		return ResourceManager::Ptr->CreatePSO(initializer);
	}

	FORCEINLINE [[nodiscard]] RootSignatureHandle CreateRootSignature(const std::string& name, const RSSpec& initializer)
	{
		return ResourceManager::Ptr->CreateRootSignature(name, initializer);
	}

	FORCEINLINE [[nodiscard]] PSOHandle CreatePSO(const RTPipelineStateSpec& initializer)
	{
		return ResourceManager::Ptr->CreatePSO(initializer);
	}

	FORCEINLINE void DestroyBuffer(BufferHandle handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyBuffer(handle, bImmediate);
	}

	FORCEINLINE void DestroyShader(ShaderHandle handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyShader(handle, bImmediate);
	}

	FORCEINLINE void DestroyTexture(TextureHandle handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyTexture(handle, bImmediate);
	}

	FORCEINLINE void DestroyPSO(PSOHandle handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyPSO(handle, bImmediate);
	}

	FORCEINLINE void DestroyRootSignature(RootSignatureHandle rs, bool bImmediate = false)
	{
		ensure(rs.IsValid());
		ResourceManager::Ptr->DestroyRootSignature(rs, bImmediate);
	}
}

#define RM_GET(ResourceHandle) limbo::RHI::ResourceManager::Ptr->Get(ResourceHandle)
