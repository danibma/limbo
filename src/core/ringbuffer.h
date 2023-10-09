#pragma once

#include <mutex>

namespace limbo::Core
{
    template <typename T, size_t Capacity, bool ThreadSafe = true>
    class RingBuffer
    {
    public:
        // Returns true if succesful
        // Returns false if there is not enough space
        inline bool PushBack(const T& item)
        {
            bool result = false;
            if constexpr(ThreadSafe)
				m_Lock.lock();
            size_t next = (m_Head + 1) % Capacity;
            if (next != m_Tail)
            {
                m_Data[m_Head] = item;
                m_Head = next;
                result = true;
            }
            if constexpr (ThreadSafe)
				m_Lock.unlock();
            return result;
        }

        // Returns true if succesful
        // Returns false if there are no items
        inline bool PopFront(T& item)
        {
            bool result = false;
            if constexpr (ThreadSafe)
				m_Lock.lock();
            if (m_Tail != m_Head)
            {
                item = m_Data[m_Tail];
                m_Tail = (m_Tail + 1) % Capacity;
                result = true;
            }
            if constexpr (ThreadSafe)
				m_Lock.unlock();
            return result;
        }

    private:
        T m_Data[Capacity];
        size_t m_Head = 0;
        size_t m_Tail = 0;
        std::mutex m_Lock; // this just works better than a spinlock here (on windows)
    };
}