#pragma once

#include "resourcemanager.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"
#include "sampler.h"

namespace limbo::Gfx
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

		Handle<Texture>				EmptyTexture;

	public:
		ResourceManager();
		virtual ~ResourceManager();

		Handle<Buffer> CreateBuffer(const BufferSpec& spec);
		Handle<Shader> CreateShader(const ShaderSpec& spec);
		Handle<Texture> CreateTextureFromFile(const char* path, const char* debugName);
		Handle<Texture> CreateTexture(const TextureSpec& spec);
		Handle<Texture> CreateTexture(ID3D12Resource* resource, const TextureSpec& spec);
		Handle<Sampler> CreateSampler(const D3D12_SAMPLER_DESC& spec);

		Buffer* GetBuffer(Handle<Buffer> buffer);
		Shader* GetShader(Handle<Shader> shader);
		Texture* GetTexture(Handle<Texture> texture);
		Sampler* GetSampler(Handle<Sampler> sampler);

		void DestroyBuffer(Handle<Buffer> buffer);
		void DestroyShader(Handle<Shader> shader);
		void DestroyTexture(Handle<Texture> texture);
		void DestroySampler(Handle<Sampler> sampler);

		void RunDeletionQueue();
		void ForceDeletionQueue();

	private:
		Pool<Buffer, 2048>		m_Buffers;
		Pool<Texture, 512>		m_Textures;
		Pool<Shader,  128>		m_Shaders;
		Pool<Sampler,   8>		m_Samplers;

		bool					m_bOnShutdown = false;

		std::deque<Deletion>	m_DeletionQueue;
	};


	// Global definitions
	inline Handle<Buffer> CreateBuffer(const BufferSpec& spec)
	{
		return ResourceManager::Ptr->CreateBuffer(spec);
	}

	inline Handle<Shader> CreateShader(const ShaderSpec& spec)
	{
		return ResourceManager::Ptr->CreateShader(spec);
	}

	inline Handle<Texture> CreateTextureFromFile(const char* path, const char* debugName)
	{
		return ResourceManager::Ptr->CreateTextureFromFile(path, debugName);
	}

	inline Handle<Texture> CreateTexture(const TextureSpec& spec)
	{
		return ResourceManager::Ptr->CreateTexture(spec);
	}

	inline Handle<Texture> CreateTexture(ID3D12Resource* resource, const TextureSpec& spec)
	{
		return ResourceManager::Ptr->CreateTexture(resource, spec);
	}

	inline Handle<Sampler> CreateSampler(const D3D12_SAMPLER_DESC& spec)
	{
		return ResourceManager::Ptr->CreateSampler(spec);
	}

	inline void DestroyBuffer(Handle<Buffer> handle)
	{
		ResourceManager::Ptr->DestroyBuffer(handle);
	}

	inline void DestroyShader(Handle<Shader> handle)
	{
		ResourceManager::Ptr->DestroyShader(handle);
	}

	inline void DestroyTexture(Handle<Texture> handle)
	{
		ResourceManager::Ptr->DestroyTexture(handle);
	}

	inline void DestroySampler(Handle<Sampler> sampler)
	{
		ResourceManager::Ptr->DestroySampler(sampler);
	}

	inline Handle<Texture> GetShaderRT(Handle<Shader> shader, uint8 rtIndex)
	{
		Shader* renderTargetShader = ResourceManager::Ptr->GetShader(shader);
		ensure(renderTargetShader);

		if (rtIndex >= renderTargetShader->RTCount)
		{
			LB_WARN("Failed to get render target index '%d'!", rtIndex);
			return Handle<Texture>();
		}

		return renderTargetShader->RenderTargets[rtIndex].Texture;
	}

	inline Handle<Texture> GetShaderDepthTarget(Handle<Shader> shader)
	{
		Shader* renderTargetShader = ResourceManager::Ptr->GetShader(shader);
		ensure(renderTargetShader);
		return renderTargetShader->DepthTarget.Texture;
	}

	inline void SetParameter(Handle<Shader> shader, const char* parameterName, Handle<Texture> texture, uint32 mipLevel = ~0)
	{
		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetTexture(parameterName, texture, mipLevel);
	}

	inline void SetParameter(Handle<Shader> shader, const char* parameterName, Handle<Buffer> buffer)
	{
		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetBuffer(parameterName, buffer);
	}

	inline void SetParameter(Handle<Shader> shader, const char* parameterName, Handle<Sampler> sampler)
	{
		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetSampler(parameterName, sampler);
	}

	// Used to bind render targets, from other shaders, as a texture
	inline void SetParameter(Handle<Shader> shader, const char* parameterName, Handle<Shader> rtShader, uint8 rtIndex)
	{
		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);

		s->SetTexture(parameterName, GetShaderRT(rtShader, rtIndex), 0);
	}

	template<typename T>
	inline void SetParameter(Handle<Shader> shader, const char* parameterName, const T& value)
	{
		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetConstant(parameterName, &value);
	}
}
