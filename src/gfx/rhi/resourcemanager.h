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
		Handle<Sampler>				DefaultLinearClamp;
		Handle<Sampler>				DefaultLinearWrap;
		Handle<Sampler>				DefaultPointClamp;

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

		void Map(Handle<Buffer> buffer, void** data, uint32 subresource = 0, D3D12_RANGE* range = nullptr);
		void Unmap(Handle<Buffer> buffer, uint32 subresource = 0, D3D12_RANGE* range = nullptr);

		// Used for Imgui images
		uint64 GetTextureID(Handle<Texture> texture);

		void DestroyBuffer(Handle<Buffer> buffer, bool bImmediate = false);
		void DestroyShader(Handle<Shader> shader, bool bImmediate = false);
		void DestroyTexture(Handle<Texture> texture, bool bImmediate = false);
		void DestroySampler(Handle<Sampler> sampler, bool bImmediate = false);

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

	inline void DestroyBuffer(Handle<Buffer> handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyBuffer(handle, bImmediate);
	}

	inline void DestroyShader(Handle<Shader> handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyShader(handle, bImmediate);
	}

	inline void DestroyTexture(Handle<Texture> handle, bool bImmediate = false)
	{
		ensure(handle.IsValid());
		ResourceManager::Ptr->DestroyTexture(handle, bImmediate);
	}

	inline void DestroySampler(Handle<Sampler> sampler, bool bImmediate = false)
	{
		ensure(sampler.IsValid());
		ResourceManager::Ptr->DestroySampler(sampler, bImmediate);
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

	inline Buffer* GetBuffer(Handle<Buffer> buffer)
	{
		Buffer* b = ResourceManager::Ptr->GetBuffer(buffer);
		FAILIF(!b, nullptr);
		return b;
	}

	inline void Map(Handle<Buffer> buffer, void** data, uint32 subresource = 0, D3D12_RANGE* range = nullptr)
	{
		ResourceManager::Ptr->Map(buffer, data, subresource, range);
	}

	inline void Unmap(Handle<Buffer> buffer, uint32 subresource = 0, D3D12_RANGE* range = nullptr)
	{
		ResourceManager::Ptr->Unmap(buffer, subresource, range);
	}

	inline void SetParameter(Handle<Shader> shader, const char* parameterName, class AccelerationStructure* accelerationStructure)
	{
		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetAccelerationStructure(parameterName, accelerationStructure);
	}

	inline void SetParameter(Handle<Shader> shader, const char* parameterName, Handle<Texture> texture, uint32 mipLevel = ~0)
	{
		if (!texture.IsValid())
		{
			LB_WARN("Tried settings parameter '%s' but given handle is not valid!", parameterName);
			return;
		}

		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetTexture(parameterName, texture, mipLevel);
	}

	inline void SetParameter(Handle<Shader> shader, const char* parameterName, Handle<Buffer> buffer)
	{
		if (!buffer.IsValid())
		{
			LB_WARN("Tried settings parameter '%s' but given handle is not valid!", parameterName);
			return;
		}

		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetBuffer(parameterName, buffer);
	}

	inline void SetParameter(Handle<Shader> shader, const char* parameterName, Handle<Sampler> sampler)
	{
		if (!sampler.IsValid())
		{
			LB_WARN("Tried settings parameter '%s' but given handle is not valid!", parameterName);
			return;
		}

		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetSampler(parameterName, sampler);
	}

	// Used to bind render targets, from other shaders, as a texture
	inline void SetParameter(Handle<Shader> shader, const char* parameterName, Handle<Shader> rtShader, uint8 rtIndex)
	{
		if (!rtShader.IsValid())
		{
			LB_WARN("Tried settings parameter '%s' but given handle is not valid!", parameterName);
			return;
		}

		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);

		s->SetTexture(parameterName, GetShaderRT(rtShader, rtIndex), 0);
	}

	template<typename T>
	inline void SetParameter(Handle<Shader> shader, const char* parameterName, const T& value)
	{
		ensure(sizeof(T) % sizeof(uint32) == 0); // the constant has to be 4 bytes

		Shader* s = ResourceManager::Ptr->GetShader(shader);
		FAILIF(!s);
		s->SetConstant(parameterName, &value);
	}

	inline Handle<Sampler> GetDefaultLinearWrapSampler()
	{
		return ResourceManager::Ptr->DefaultLinearWrap;
	}

	inline Handle<Sampler> GetDefaultLinearClampSampler()
	{
		return ResourceManager::Ptr->DefaultLinearClamp;
	}

	inline Handle<Sampler> GetDefaultPointClampSampler()
	{
		return ResourceManager::Ptr->DefaultPointClamp;
	}

	// This can be used as a TextureID for ImGui
	inline uint64 GetTextureID(Handle<Texture> texture)
	{
		return ResourceManager::Ptr->GetTextureID(texture);
	}

	inline uint64 GetShaderRTTextureID(Handle<Shader> shader, uint8 rtIndex)
	{
		return ResourceManager::Ptr->GetTextureID(GetShaderRT(shader, rtIndex));
	}

	inline uint64 GetShaderDTTextureID(Handle<Shader> shader)
	{
		return ResourceManager::Ptr->GetTextureID(GetShaderDepthTarget(shader));
	}
}
