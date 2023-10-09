#pragma once

namespace limbo
{
	template<typename T>
	class Span
	{
	public:
		Span() :
			m_Data(nullptr), m_Size(0)
		{}

		Span(const std::initializer_list<std::remove_cv_t<T>>& list) :
			m_Data(list.begin()), m_Size((uint32)list.size())
		{}

		Span(const std::vector<std::remove_cv_t<T>>& list) :
			m_Data(list.data()), m_Size((uint32)list.size())
		{}

		Span(const T* pValue, uint32 size) :
			m_Data(pValue), m_Size(size)
		{}

		template<size_t N>
		Span(const T(&arr)[N])
			: m_Data(arr), m_Size(N)
		{}

		Span(const T& value) :
			m_Data(&value), m_Size(1)
		{}

		const T& operator[](uint32 idx) const
		{
			check(idx < m_Size);
			return m_Data[idx];
		}

		const T* begin() const { return m_Data; }
		const T* end() const { return m_Data + m_Size; }

		uint32 GetSize() const
		{
			return m_Size;
		}

	private:
		const T* m_Data;
		uint32 m_Size;
	};

	template<typename Type, uint32 Capacity>
	class TStaticArray
	{
	public:
		TStaticArray()
			: m_Capacity(Capacity)
			, m_Size(0)
		{
		}

		TStaticArray(const std::initializer_list<std::remove_cv_t<Type>>& list)
			: m_Capacity(Capacity)
			, m_Size((uint32)list.size())
		{
			memcpy(m_Data, list.begin(), sizeof(Type) * list.size());
		}

		TStaticArray(const Span<std::remove_cv_t<Type>>& span)
			: m_Capacity(Capacity)
			, m_Size(span.GetSize())
		{
			memcpy(m_Data, span.begin(), sizeof(Type) * span.GetSize());
		}

		template<typename OtherType, uint32 OtherCapacity>
		TStaticArray<Type, Capacity>& operator=(const TStaticArray<std::remove_cv_t<OtherType>, OtherCapacity>& other)
		{
			static_assert(TIsSame<Type, OtherType>::Value);
			static_assert(OtherCapacity > Capacity);

			m_Capacity = OtherCapacity;
			m_Size = other.GetSize();
			memcpy(m_Data, other.begin(), sizeof(Type) * other.GetSize());
		}

		uint32 GetSize() const
		{
			return m_Size;
		}

		uint32 GetCapacity() const
		{
			return m_Capacity;
		}

		const Type* GetData() const
		{
			return m_Data;
		}

		Type& operator[](uint32 index)
		{
			return m_Data[index];
		}

		const Type& operator[](uint32 index) const
		{
			return m_Data[index];
		}

		void Add(const Type& element)
		{
			check(m_Size < m_Capacity);
			m_Data[m_Size] = element;
			m_Size++;
		}

		const Type* begin() const { return m_Data; }
		const Type* end() const { return m_Data + m_Size; }

	private:
		Type		m_Data[Capacity];
		uint32		m_Capacity;
		uint32		m_Size;
	};
}