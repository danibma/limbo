#pragma once

#include "resourcepool.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"

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

		template<typename ResourceType>
		ResourceType* Get(Handle<ResourceType> resourceHandle)
		{
			if constexpr (TIsSame<ResourceType, Texture>::Value)
				return m_Textures.Get(resourceHandle);
			else if constexpr (TIsSame<ResourceType, Buffer>::Value)
				return m_Buffers.Get(resourceHandle);
			else if constexpr (TIsSame<ResourceType, Shader>::Value)
				return m_Shaders.Get(resourceHandle);
		}

		void DestroyBuffer(BufferHandle buffer, bool bImmediate = false);
		void DestroyShader(ShaderHandle shader, bool bImmediate = false);
		void DestroyTexture(TextureHandle texture, bool bImmediate = false);

		void RunDeletionQueue();
		void ForceDeletionQueue();

	private:
		Pool<Buffer,			  1<<7>		m_Buffers;
		Pool<Texture,			  1<<7>	    m_Textures;
		Pool<Shader,			  128>		m_Shaders;

		bool						m_bOnShutdown = false;

		std::deque<Deletion>		m_DeletionQueue;
	};

	// Global definitions
	inline BufferHandle CreateBuffer(const BufferSpec& spec)
	{
		return ResourceManager::Ptr->CreateBuffer(spec);
	}

	inline ShaderHandle CreateShader(const char* file, const char* entryPoint, ShaderType type)
	{
		return ResourceManager::Ptr->CreateShader(file, entryPoint, type);
	}

	inline TextureHandle CreateTextureFromFile(const char* path, const char* debugName)
	{
		return ResourceManager::Ptr->CreateTextureFromFile(path, debugName);
	}

	inline TextureHandle CreateTexture(const TextureSpec& spec)
	{
		return ResourceManager::Ptr->CreateTexture(spec);
	}

	inline TextureHandle CreateTexture(ID3D12Resource* resource, const TextureSpec& spec)
	{
		return ResourceManager::Ptr->CreateTexture(resource, spec);
	}

	inline void DestroyBuffer(BufferHandle handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyBuffer(handle, bImmediate);
	}

	inline void DestroyShader(ShaderHandle handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyShader(handle, bImmediate);
	}

	inline void DestroyTexture(TextureHandle handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyTexture(handle, bImmediate);
	}
}

#define RM_GET(ResourceHandle) limbo::RHI::ResourceManager::Ptr->Get(ResourceHandle)
