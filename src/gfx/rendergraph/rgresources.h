#pragma once
#include "gfx/rhi/buffer.h"
#include "gfx/rhi/resourcehandle.h"
#include "gfx/rhi/texture.h"

namespace limbo::Gfx
{
	enum class RGResourceType
	{
		Texture,
		Buffer,
	};

	template<typename T> struct RGResourceTypeTraits { };

	template<>
	struct RGResourceTypeTraits<RHI::Texture>
	{
		constexpr static RGResourceType Type = RGResourceType::Texture;
		using TSpec = RHI::TextureSpec;
	};

	template<>
	struct RGResourceTypeTraits<RHI::Buffer>
	{
		constexpr static RGResourceType Type = RGResourceType::Buffer;
		using TSpec = RHI::BufferSpec;
	};

	class RGResource
	{
	public:
		RGResource(const char* name, RGResourceType type, RHI::Handle handle = RHI::Handle())
			: m_Type(type), m_Name(name), m_ResourceHandle(handle) {}

	private:
		RGResourceType m_Type;
		std::string m_Name;
		RHI::Handle m_ResourceHandle;
	};

	template<typename T>
	class RGResourceTyped : public RGResource
	{
	public:
		using TSpec = typename RGResourceTypeTraits<T>::TSpec;

		RGResourceTyped(const char* name, const TSpec& spec, RHI::Handle handle = RHI::Handle())
			: RGResource(name, RGResourceTypeTraits<T>::Type, handle), m_Spec(spec) {}

		const TSpec& GetSpec() const { return m_Spec; }

	private:
		TSpec m_Spec;
	};

	typedef RGResourceTyped<RHI::Texture> RGTexture;
	typedef RGResourceTyped<RHI::Buffer> RGBuffer;

	/**
	 * This represents an index in the RGRegistry resources list
	 */
	struct RGHandle
	{
		static constexpr uint16 INVALID_INDEX = 0xFFFF;
		uint16 Index = INVALID_INDEX;
	};
}
