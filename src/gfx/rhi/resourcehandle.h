#pragma once

#include "core/core.h"

namespace limbo::RHI
{
	class Handle
	{
	public:
		Handle() : m_Index(0xFFFF), m_Generation(0) {}
		bool IsValid() const { return m_Index != 0xFFFF; }

		bool operator!=(const Handle& other)
		{
			return !(m_Index == other.m_Index && m_Generation == other.m_Generation);
		}

	private:
		Handle(uint16 index, uint16 generation) : m_Index(index), m_Generation(generation) {}

		uint16 m_Index;
		uint16 m_Generation;

		template<typename T1, uint16 InitialSize> friend class Pool;
	};

	/**
	 * This is a typed version of the handle above.
	 * Just used for type safety.
	 */
	template<typename T>
	struct TypedHandle
	{
		TypedHandle() = default;
		TypedHandle(Handle handle) : RawHandle(handle) {}

		bool IsValid() const { return RawHandle.IsValid(); }

		bool operator!=(const Handle& other)
		{
			return RawHandle != other;
		}

		bool operator!=(const TypedHandle<T>& other)
		{
			return RawHandle != other.RawHandle;
		}

		Handle RawHandle;
	};

	/**
	 * Casts a non-typed handle into a typed handle.
	 */
	template<typename T>
	FORCEINLINE NODISCARD TypedHandle<T> Cast(Handle handle)
	{
		return TypedHandle<T>(handle);
	}
}