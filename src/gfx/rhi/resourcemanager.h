#pragma once

#include "resourcepool.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"
#include "pipelinestateobject.h"

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
		PSOHandle CreatePSO(const PipelineStateInitializer& initializer);
		PSOHandle CreatePSO(const RaytracingPipelineStateInitializer& initializer);

		template<typename ResourceType>
		ResourceType* Get(Handle<ResourceType> resourceHandle)
		{
			if constexpr (TIsSame<ResourceType, Texture>::Value)
				return m_Textures.Get(resourceHandle);
			else if constexpr (TIsSame<ResourceType, Buffer>::Value)
				return m_Buffers.Get(resourceHandle);
			else if constexpr (TIsSame<ResourceType, Shader>::Value)
				return m_Shaders.Get(resourceHandle);
			else if constexpr (TIsSame<ResourceType, PipelineStateObject>::Value)
				return m_PSOs.Get(resourceHandle);
		}

		void DestroyBuffer(BufferHandle buffer, bool bImmediate = false);
		void DestroyShader(ShaderHandle shader, bool bImmediate = false);
		void DestroyTexture(TextureHandle texture, bool bImmediate = false);
		void DestroyPSO(PSOHandle pso, bool bImmediate = false);

		void RunDeletionQueue();
		void ForceDeletionQueue();

	private:
		Pool<Buffer,			  1<<7>		m_Buffers;
		Pool<Texture,			  1<<7>	    m_Textures;
		Pool<Shader,			  128>		m_Shaders;
		Pool<PipelineStateObject, 32>		m_PSOs;

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

	FORCEINLINE [[nodiscard]] PSOHandle CreatePSO(const PipelineStateInitializer& initializer)
	{
		return ResourceManager::Ptr->CreatePSO(initializer);
	}

	FORCEINLINE [[nodiscard]] PSOHandle CreatePSO(const RaytracingPipelineStateInitializer& initializer)
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
}

#define RM_GET(ResourceHandle) limbo::RHI::ResourceManager::Ptr->Get(ResourceHandle)
