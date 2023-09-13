#pragma once
#include "rhi/resourcepool.h"

namespace limbo
{
	extern class GPUProfiler GGPUProfiler;
	extern class CPUProfiler GCPUProfiler;

	namespace RHI
	{
		class CommandContext;
		class Buffer;
	}

	class GPUProfiler
	{
	private:
		RHI::Handle<RHI::Buffer>	m_Readback;
		ComPtr<ID3D12QueryHeap>		m_QueryHeap;
		double						m_AvgRenderTime = 0;

	public:
		void Initialize();
		void Shutdown();

		void StartProfile(RHI::CommandContext* cmd, const char* name);
		void EndProfile(RHI::CommandContext* cmd, const char* name);

		double GetRenderTime() const
		{
			return m_AvgRenderTime;
		}

		void EndFrame();
	};

	class CPUProfiler
	{
	private:
		Core::Timer m_Timer;
		double		m_AvgRenderTime = 0;

	public:
		void Initialize();
		void Shutdown();

		void StartProfile(const char* name);
		void EndProfile(const char* name);

		double GetRenderTime() const
		{
			return m_AvgRenderTime;
		}

		void EndFrame();
	};

	template<typename ProfilerType>
	class ScopedProfile
	{
		RHI::CommandContext* m_Context;
		std::string			 m_Name;

	public:
		ScopedProfile(RHI::CommandContext* cmd, const char* name)
			: m_Context(cmd), m_Name(name)
		{
			GGPUProfiler.StartProfile(m_Context, m_Name.c_str());
		}

		ScopedProfile(const char* name)
			: m_Context(nullptr), m_Name(name)
		{
			GCPUProfiler.StartProfile(m_Name.c_str());
		}

		~ScopedProfile()
		{
			if constexpr (TIsSame<ProfilerType, GPUProfiler>::Value)
				GGPUProfiler.EndProfile(m_Context, m_Name.c_str());
			else
				GCPUProfiler.EndProfile(m_Name.c_str());
		}
	};

	namespace Profiler
	{
		inline void Initialize()
		{
			GCPUProfiler.Initialize();
			GGPUProfiler.Initialize();
		}

		inline void Shutdown()
		{
			GCPUProfiler.Shutdown();
			GGPUProfiler.Shutdown();
		}

		inline void Present()
		{
			GCPUProfiler.EndFrame();
			GGPUProfiler.EndFrame();
		}
	}
}

#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )

// GPU
#define PROFILE_GPU_SCOPE(CommandContext, Name) limbo::ScopedProfile<GPUProfiler> MACRO_CONCAT(__s__, __COUNTER__)(CommandContext, Name)
#define PROFILE_GPU_BEGIN(CommandContext, Name) limbo::GGPUProfiler.StartProfile(CommandContext, Name)
#define PROFILE_GPU_END(CommandContext, Name)   limbo::GGPUProfiler.EndProfile(CommandContext, Name)

// CPU
#define PROFILE_CPU_SCOPE(Name) limbo::ScopedProfile<CPUProfiler> MACRO_CONCAT(__s__, __COUNTER__)(Name)
#define PROFILE_CPU_BEGIN(Name) limbo::GCPUProfiler.StartProfile(Name)
#define PROFILE_CPU_END(Name)   limbo::GCPUProfiler.EndProfile(Name)

// Both
#define PROFILE_SCOPE(CommandContext, Name) PROFILE_CPU_SCOPE(Name); PROFILE_GPU_SCOPE(CommandContext, Name)
#define PROFILE_BEGIN(CommandContext, Name) PROFILE_CPU_BEGIN(Name); PROFILE_GPU_BEGIN(CommandContext, Name)
#define PROFILE_END(CommandContext, Name)   PROFILE_CPU_END(Name);   PROFILE_GPU_END(CommandContext, Name)
