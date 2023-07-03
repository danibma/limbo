#pragma once

#include "definitions.h"
#include "resourcepool.h"

namespace limbo
{
	class Texture;
	class Buffer;
	struct BindGroupSpec
	{
	private:
		struct TextureSlot
		{
			uint32 slot;
			Handle<Texture> texture;
		};

		struct BufferSlot
		{
			uint32 slot;
			Handle<Buffer> buffer;
		};

	public:
		std::vector<TextureSlot> textures;
		std::vector<BufferSlot>  buffers;
	};

	class BindGroup
	{
	public:
		virtual ~BindGroup() = default;
	};

	struct ShaderSpec
	{
		const char*						programName;
		const char*						entryPoint;
		std::vector<Handle<BindGroup>>	bindGroups;
		ShaderType						type = ShaderType::Graphics;
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;
	};
}
