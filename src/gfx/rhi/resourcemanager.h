#pragma once

#include "resourcemanager.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"
#include "sampler.h"

namespace limbo::gfx
{
	class ResourceManager
	{
	public:
		static ResourceManager* ptr;

	public:
		ResourceManager() = default;
		virtual ~ResourceManager();

		Handle<Buffer> createBuffer(const BufferSpec& spec);
		Handle<Shader> createShader(const ShaderSpec& spec);
		Handle<Texture> createTexture(const TextureSpec& spec);
		Handle<Texture> createTexture(ID3D12Resource* resource, const TextureSpec& spec);
		Handle<Sampler> createSampler(const D3D12_SAMPLER_DESC& spec);

		Buffer* getBuffer(Handle<Buffer> buffer);
		Shader* getShader(Handle<Shader> shader);
		Texture* getTexture(Handle<Texture> texture);
		Sampler* getSampler(Handle<Sampler> sampler);

		void destroyBuffer(Handle<Buffer> buffer);
		void destroyShader(Handle<Shader> shader);
		void destroyTexture(Handle<Texture> texture);
		void destroySampler(Handle<Sampler> sampler);

	private:
		Pool<Buffer> m_buffers;
		Pool<Shader> m_shaders;
		Pool<Texture> m_textures;
		Pool<Sampler> m_samplers;
	};


	// Global definitions
	inline Handle<Buffer> createBuffer(const BufferSpec& spec)
	{
		return ResourceManager::ptr->createBuffer(spec);
	}

	inline Handle<Shader> createShader(const ShaderSpec& spec)
	{
		return ResourceManager::ptr->createShader(spec);
	}

	inline Handle<Texture> createTexture(const TextureSpec& spec)
	{
		return ResourceManager::ptr->createTexture(spec);
	}

	inline Handle<Texture> createTexture(ID3D12Resource* resource, const TextureSpec& spec)
	{
		return ResourceManager::ptr->createTexture(resource, spec);
	}

	inline Handle<Sampler> createSampler(const D3D12_SAMPLER_DESC& spec)
	{
		return ResourceManager::ptr->createSampler(spec);
	}

	inline void destroyBuffer(Handle<Buffer> handle)
	{
		ResourceManager::ptr->destroyBuffer(handle);
	}

	inline void destroyShader(Handle<Shader> handle)
	{
		ResourceManager::ptr->destroyShader(handle);
	}

	inline void destroyTexture(Handle<Texture> handle)
	{
		ResourceManager::ptr->destroyTexture(handle);
	}

	inline void destroySampler(Handle<Sampler> sampler)
	{
		ResourceManager::ptr->destroySampler(sampler);
	}

	inline void setParameter(Handle<Shader> shader, const char* parameterName, Handle<Texture> texture)
	{
		Shader* s = ResourceManager::ptr->getShader(shader);
		FAILIF(!s);
		s->setTexture(parameterName, texture);
	}

	inline void setParameter(Handle<Shader> shader, const char* parameterName, Handle<Buffer> buffer)
	{
		Shader* s = ResourceManager::ptr->getShader(shader);
		FAILIF(!s);
		s->setBuffer(parameterName, buffer);
	}

	inline void setParameter(Handle<Shader> shader, const char* parameterName, Handle<Sampler> sampler)
	{
		Shader* s = ResourceManager::ptr->getShader(shader);
		FAILIF(!s);
		s->setSampler(parameterName, sampler);
	}

	// Used to bind render targets, from other shaders, as a texture
	inline void setParameter(Handle<Shader> shader, const char* parameterName, Handle<Shader> rtShader, uint8 rtIndex)
	{
		Shader* s = ResourceManager::ptr->getShader(shader);
		FAILIF(!s);
		Shader* renderTargetShader = ResourceManager::ptr->getShader(rtShader);
		FAILIF(!renderTargetShader);

		if (rtIndex >= renderTargetShader->rtCount)
		{
			LB_WARN("Failed to set parameter '%s' because the render target index '%d' is not valid!", parameterName, rtIndex);
			return;
		}

		s->setTexture(parameterName, renderTargetShader->renderTargets[rtIndex]);
	}

	template<typename T>
	inline void setParameter(Handle<Shader> shader, const char* parameterName, const T& value)
	{
		Shader* s = ResourceManager::ptr->getShader(shader);
		FAILIF(!s);
		s->setConstant(parameterName, &value);
	}
}
