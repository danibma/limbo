#pragma once
#include "rhi/resourcepool.h"

namespace limbo
{
	extern class GPUProfiler GGPUProfiler;

	namespace Gfx
	{
		class CommandContext;
		class Buffer;
	}

	class GPUProfiler
	{
	private:
		Gfx::Handle<Gfx::Buffer>	m_Readback;
		ComPtr<ID3D12QueryHeap>		m_QueryHeap;
		double						m_AvgRenderTime = 0;

	public:
		void Initialize();
		void Shutdown();

		void StartProfile(Gfx::CommandContext* cmd, const char* name);
		void EndProfile(Gfx::CommandContext* cmd, const char* name);

		double GetRenderTime() const
		{
			return m_AvgRenderTime;
		}

		void EndFrame();
	};

	template<typename ProfilerType>
	class ScopedProfile
	{
		Gfx::CommandContext* m_Context;
		std::string			 m_Name;

	public:
		ScopedProfile(Gfx::CommandContext* cmd, const char* name)
			: m_Context(cmd), m_Name(name)
		{
			if constexpr (TIsSame<ProfilerType, GPUProfiler>::Value)
				GGPUProfiler.StartProfile(m_Context, m_Name.c_str());
		}

		~ScopedProfile()
		{
			if constexpr (TIsSame<ProfilerType, GPUProfiler>::Value)
				GGPUProfiler.EndProfile(m_Context, m_Name.c_str());
		}
	};

	namespace Profiler
	{
		inline void Initialize()
		{
			GGPUProfiler.Initialize();
		}

		inline void Shutdown()
		{
			GGPUProfiler.Shutdown();
		}
	}
}

#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )

#define PROFILE_GPU_SCOPE(CommandContext, Name) limbo::ScopedProfile<GPUProfiler> MACRO_CONCAT(__s__, __COUNTER__)(CommandContext, Name)
#define PROFILE_GPU_BEGIN(CommandContext, Name) limbo::GGPUProfiler.StartProfile(CommandContext, Name)
#define PROFILE_GPU_END(CommandContext, Name) limbo::GGPUProfiler.EndProfile(CommandContext, Name)
#define PROFILE_GPU_PRESENT() limbo::GGPUProfiler.EndFrame()

//#undef CONCAT_IMPL
//#undef MACRO_CONCAT

