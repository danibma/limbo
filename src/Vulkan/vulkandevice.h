#pragma once

#include "device.h"
#include "vulkanloader.h"

namespace limbo
{
	class VulkanDevice final : public Device
	{
	public:
		VulkanDevice();
		virtual ~VulkanDevice();

		virtual void setParameter(Handle<Shader> shader, uint8 slot, const void* data) override;
		virtual void setParameter(Handle<Shader> shader, uint8 slot, Handle<Buffer> buffer) override;
		virtual void setParameter(Handle<Shader> shader, uint8 slot, Handle<Texture> texture) override;

		virtual void bindShader(Handle<Shader> shader) override;
		virtual void bindVertexBuffer(Handle<Buffer> vertexBuffer) override;
		virtual void bindIndexBuffer(Handle<Buffer> indexBuffer) override;

		virtual void draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 1, uint32 firstInstance = 1) override;

		virtual void present() override;

	};
}