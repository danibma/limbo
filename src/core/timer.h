#pragma once

namespace limbo::Core
{
	struct Timer
	{
		Timer()
		{
			Record();
		}

		// Record a reference timestamp
		inline void Record()
		{
			LARGE_INTEGER counter;
			QueryPerformanceCounter(&counter);
			m_Timestamp = counter.QuadPart;

			LARGE_INTEGER frequency;
			QueryPerformanceFrequency(&frequency);
			m_Frequency = frequency.QuadPart;
		}

		// Elapsed time in seconds since the Timer creation or last call to record()
		inline float ElapsedSeconds()
		{
			LARGE_INTEGER counter;
			QueryPerformanceCounter(&counter);

			return float(counter.QuadPart - m_Timestamp) / m_Frequency;
		}

		// Elapsed time in milliseconds since the Timer creation or last call to record()
		inline float ElapsedMilliseconds()
		{
			return ElapsedSeconds() * 1000.0f;
		}

	private:
		uint64 m_Timestamp;

		inline static uint64 m_Frequency;
	};
}
