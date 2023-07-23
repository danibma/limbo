#pragma once

#include <chrono>

namespace limbo::core
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
			m_timestamp = std::chrono::high_resolution_clock::now();
		}

		// Elapsed time in seconds since the Timer creation or last call to record()
		inline float ElapsedSeconds()
		{
			auto timestamp2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> timeSpan = std::chrono::duration_cast<std::chrono::duration<float>>(timestamp2 - m_timestamp);
			return timeSpan.count();
		}

		// Elapsed time in milliseconds since the Timer creation or last call to record()
		inline float ElapsedMilliseconds()
		{
			return ElapsedSeconds() * 1000.0f;
		}

	private:
		std::chrono::high_resolution_clock::time_point m_timestamp;
	};
}
