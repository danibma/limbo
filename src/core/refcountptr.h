#pragma once

#include "core.h"

namespace limbo
{
	/**
	 * The base class of reference counted objects.
	 */
	class RefCountedObject
	{
	public:
		RefCountedObject() : m_NumRefs(0) {}
		virtual ~RefCountedObject() { check(!m_NumRefs); }

		uint32 AddRef() const
		{
			return uint32(++m_NumRefs);
		}

		uint32 Release() const
		{
			uint32 Refs = uint32(--m_NumRefs);
			if (Refs == 0)
			{
				delete this;
			}
			return Refs;
		}

		uint32 GetRefCount() const
		{
			return uint32(m_NumRefs);
		}

	private:
		mutable int32 m_NumRefs;
	};

	/**
	 * A ref counting pointer to an object which implements AddRef/Release.
	 */
	template<typename T>
	class RefCountPtr
	{
		typedef T* PointerType;

	public:
		FORCEINLINE RefCountPtr() :
			m_Ptr(nullptr)
		{ }

		RefCountPtr(T* InReference)
		{
			m_Ptr = InReference;
			if (m_Ptr)
				m_Ptr->AddRef();
		}

		RefCountPtr(const RefCountPtr& Copy)
		{
			m_Ptr = Copy.m_Ptr;
			if (m_Ptr)
			{
				m_Ptr->AddRef();
			}
		}

		template<typename CopyType>
		explicit RefCountPtr(const RefCountPtr<CopyType>& Copy)
		{
			m_Ptr = static_cast<T*>(Copy.Get());
			if (m_Ptr)
			{
				m_Ptr->AddRef();
			}
		}

		FORCEINLINE RefCountPtr(RefCountPtr&& Move)
		{
			m_Ptr = Move.m_Ptr;
			Move.m_Ptr = nullptr;
		}

		template<typename MoveReferencedType>
		explicit RefCountPtr(RefCountPtr<MoveReferencedType>&& Move)
		{
			m_Ptr = static_cast<T*>(Move.Get());
			Move.m_Ptr = nullptr;
		}

		~RefCountPtr()
		{
			T* temp = m_Ptr;
			if (temp)
			{
				m_Ptr = nullptr;
				temp->Release();
			}
		}

		RefCountPtr& operator=(T* InReference)
		{
			// Call AddRef before Release, in case the new reference is the same as the old reference.
			T* OldReference = m_Ptr;
			m_Ptr = InReference;
			if (m_Ptr)
				m_Ptr->AddRef();

			if (OldReference)
				OldReference->Release();

			return *this;
		}

		FORCEINLINE RefCountPtr& operator=(const RefCountPtr& InPtr)
		{
			return *this = InPtr.m_Ptr;
		}

		template<typename CopyType>
		FORCEINLINE RefCountPtr& operator=(const RefCountPtr<CopyType>& InPtr)
		{
			return *this = InPtr.Get();
		}

		RefCountPtr& operator=(RefCountPtr&& InPtr)
		{
			if (this != &InPtr)
			{
				T* OldReference = m_Ptr;
				m_Ptr = InPtr.m_Ptr;
				InPtr.m_Ptr = nullptr;
				if (OldReference)
				{
					OldReference->Release();
				}
			}
			return *this;
		}

		FORCEINLINE T* operator->() const
		{
			return m_Ptr;
		}

		FORCEINLINE operator PointerType() const 
		{
			return m_Ptr;
		}

		uint32 GetRefCount()
		{
			uint32 Result = 0;
			if (m_Ptr)
			{
				Result = m_Ptr->GetRefCount();
				check(Result > 0); // you should never have a zero ref count if there is a live ref counted pointer
			}
			return Result;
		}

		FORCEINLINE void Swap(RefCountPtr& InPtr)
		{
			T* OldReference = m_Ptr;
			m_Ptr = InPtr.m_Ptr;
			InPtr.m_Ptr = OldReference;
		}

		FORCEINLINE bool IsValid() const 
		{
			return m_Ptr != nullptr;
		}

		FORCEINLINE T* Get() const
		{
			return m_Ptr;
		}

		FORCEINLINE void Reset()
		{
			if (m_Ptr)
				m_Ptr->Release();
		}

		FORCEINLINE void** GetRefVoid()
		{
			return (void**)(ReleaseAndGetAddressOf());
		}

		FORCEINLINE NODISCARD T** ReleaseAndGetAddressOf()
		{
			if (m_Ptr)
				m_Ptr->Release();
			return &m_Ptr;
		}

		FORCEINLINE NODISCARD T* const* GetAddressOf() const
		{
			return &m_Ptr;
		}

		FORCEINLINE NODISCARD T** GetAddressOf()
		{
			return &m_Ptr;
		}

	private:
		T* m_Ptr;

		template <typename OtherType>
		friend class RefCountPtr;
	};
}