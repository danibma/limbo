#include "stdafx.h"
#include "profiler.h"
#include "uirenderer.h"

#include "rhi/commandqueue.h"
#include "rhi/device.h"
#include "rhi/resourcemanager.h"

#include <set>

#define BEGIN_UI() \
	if (UI::Globals::bShowProfiler) \
	{ \
		ImGui::SetNextWindowBgAlpha(0.7f); \
		ImGui::Begin("Profiler", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize ); \
		ImGui::SetWindowPos(ImVec2(ImGui::GetMainViewport()->Size.x - ImGui::GetWindowSize().x - 5.0f, 28.0f)); \
	}

namespace limbo
{
	GPUProfiler GGPUProfiler;
	CPUProfiler GCPUProfiler;

	struct OrderedData
	{
		const char* Name;
		double		Time;

		bool operator<(const OrderedData& rhs) const
		{
			return Time < rhs.Time;
		}
	};

	struct ProfileData
	{
		std::string Name;

		int64		StartTime;
		int64		EndTime;

		static constexpr uint64 FilterSize = 64;
		double TimeSamples[FilterSize] = { };
		uint64 CurrentSample = 0;
	};

	constexpr uint64 MaxProfiles = 64;
	std::vector<ProfileData> GPUProfiles;
	std::vector<ProfileData> CPUProfiles;

	//
	// GPU
	//
	void GPUProfiler::Initialize()
	{
		GPUProfiles.reserve(MaxProfiles);

		m_Readback = RHI::CreateBuffer({
			.DebugName = "Profiler Readback buffer",
			.ByteSize = MaxProfiles * RHI::NUM_BACK_BUFFERS * 2 * sizeof(uint64),
			.Flags = RHI::BufferUsage::Readback | RHI::BufferUsage::Upload
		});

		D3D12_QUERY_HEAP_DESC desc = {
			.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
			.Count = MaxProfiles * RHI::NUM_BACK_BUFFERS * 2,
			.NodeMask = 0
		};
		DX_CHECK(RHI::Device::Ptr->GetDevice()->CreateQueryHeap(&desc, IID_PPV_ARGS(&m_QueryHeap)));
	}

	void GPUProfiler::Shutdown()
	{
		RHI::DestroyBuffer(m_Readback);
	}

	void GPUProfiler::StartProfile(RHI::CommandContext* cmd, const char* name)
	{
#if NO_LOG // don't run the profiler stuff in release mode
		return;
#endif

		ensure(cmd->GetType() != RHI::ContextType::Copy); // we don't support copy queues

		uint32 profileIndex = -1;
		for (int i = 0; i < GPUProfiles.size(); ++i)
		{
			if (GPUProfiles[i].Name == name)
			{
				profileIndex = i;
				break;
			}
		}

		if (profileIndex == -1)
		{
			profileIndex = (uint32)GPUProfiles.size();
			ProfileData& data = GPUProfiles.emplace_back();
			data.Name = name;
		}

		const uint32 startQueryIdx = uint32(profileIndex * 2);
		cmd->Get()->EndQuery(m_QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, startQueryIdx);
	}

	void GPUProfiler::EndProfile(RHI::CommandContext* cmd, const char* name)
	{
#if NO_LOG // don't run the profiler stuff in release mode
		return;
#endif

		ensure(cmd->GetType() != RHI::ContextType::Copy); // we don't support copy queues

		uint32 profileIndex = -1;
		for (int i = 0; i < GPUProfiles.size(); ++i)
		{
			if (GPUProfiles[i].Name == name)
			{
				profileIndex = i;
				break;
			}
		}

		// Insert the end timestamp
		const uint32 startQueryIdx = uint32(profileIndex * 2);
		const uint32 endQueryIdx = startQueryIdx + 1;
		cmd->Get()->EndQuery(m_QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, endQueryIdx);

		// Resolve the data
		const uint64 dstOffset = ((RHI::Device::Ptr->GetCurrentFrameIndex() * MaxProfiles * 2) + startQueryIdx) * sizeof(uint64);
		cmd->Get()->ResolveQueryData(m_QueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, startQueryIdx, 2, RHI::GetBuffer(m_Readback)->Resource.Get(), dstOffset);
	}

	void GPUProfiler::EndFrame()
	{
#if NO_LOG // don't run the profiler stuff in release mode
		return;
#endif

		uint64 gpuFrequency = 0;
		RHI::Device::Ptr->GetCommandQueue(RHI::ContextType::Direct)->GetTimestampFrequency(&gpuFrequency);

		const uint64* frameQueryData = nullptr;
		RHI::Map(m_Readback);
		const uint64* queryData = (uint64*)RHI::GetMappedData(m_Readback);
		frameQueryData = queryData + (RHI::Device::Ptr->GetCurrentFrameIndex() * MaxProfiles * 2);

		BEGIN_UI()
		if (UI::Globals::bShowProfiler)
			ImGui::SeparatorText("GPU Times");

		std::set<OrderedData> orderedData;

		for (int profileIndex = 0; profileIndex < GPUProfiles.size(); ++profileIndex)
		{
			ProfileData& profileData = GPUProfiles[profileIndex];
			profileData.StartTime = frameQueryData[profileIndex * 2 + 0];
			profileData.EndTime	  = frameQueryData[profileIndex * 2 + 1];

			uint64 delta = profileData.EndTime - profileData.StartTime;
			double time = (delta / double(gpuFrequency)) * 1000.0f;

			profileData.TimeSamples[profileData.CurrentSample] = time;
			profileData.CurrentSample = (profileData.CurrentSample + 1) % ProfileData::FilterSize;

			double maxTime = 0.0;
			double avgTime = 0.0;
			uint64 avgTimeSamples = 0;
			for (UINT i = 0; i < ProfileData::FilterSize; ++i)
			{
				if (profileData.TimeSamples[i] <= 0.0)
					continue;
				maxTime = Math::Max(profileData.TimeSamples[i], maxTime);
				avgTime += profileData.TimeSamples[i];
				++avgTimeSamples;
			}

			if (avgTimeSamples > 0)
				avgTime /= double(avgTimeSamples);

			if (profileData.Name == "Render")
				m_AvgRenderTime = avgTime;

			if (UI::Globals::bShowProfiler)
				orderedData.emplace(profileData.Name.c_str(), avgTime);
		}

		if (UI::Globals::bShowProfiler)
		{
			for (const OrderedData& data : orderedData)
				ImGui::Text("%s: %.2fms", data.Name, data.Time);

			ImGui::End();
		}
	}


	//
	// CPU
	//
	void CPUProfiler::Initialize()
	{
	}

	void CPUProfiler::Shutdown()
	{
	}

	void CPUProfiler::StartProfile(const char* name)
	{
#if NO_LOG // don't run the profiler stuff in release mode
		return;
#endif

		uint32 profileIndex = -1;
		for (int i = 0; i < CPUProfiles.size(); ++i)
		{
			if (CPUProfiles[i].Name == name)
			{
				profileIndex = i;
				break;
			}
		}

		if (profileIndex == -1)
		{
			profileIndex = (uint32)CPUProfiles.size();
			CPUProfiles.emplace_back();
		}

		ProfileData& data = CPUProfiles[profileIndex];
		data.Name			= name;
		data.StartTime		= (int64)(m_Timer.ElapsedMilliseconds() * 1000);
	}

	void CPUProfiler::EndProfile(const char* name)
	{
#if NO_LOG // don't run the profiler stuff in release mode
		return;
#endif

		uint32 profileIndex = -1;
		for (int i = 0; i < CPUProfiles.size(); ++i)
		{
			if (CPUProfiles[i].Name == name)
			{
				profileIndex = i;
				break;
			}
		}
		check(profileIndex != -1);

		ProfileData& data = CPUProfiles[profileIndex];
		data.EndTime = (int64)(m_Timer.ElapsedMilliseconds() * 1000);
	}

	void CPUProfiler::EndFrame()
	{
#if NO_LOG // don't run the profiler stuff in release mode
		return;
#endif

		BEGIN_UI()
		if (UI::Globals::bShowProfiler)
			ImGui::SeparatorText("CPU Times");

		std::set<OrderedData> orderedData;

		for (int profileIndex = 0; profileIndex < CPUProfiles.size(); ++profileIndex)
		{
			ProfileData& profileData = CPUProfiles[profileIndex];

			uint64 delta = profileData.EndTime - profileData.StartTime;
			double time = double(delta) / 1000.0f;

			profileData.TimeSamples[profileData.CurrentSample] = time;
			profileData.CurrentSample = (profileData.CurrentSample + 1) % ProfileData::FilterSize;

			double maxTime = 0.0;
			double avgTime = 0.0;
			uint64 avgTimeSamples = 0;
			for (UINT i = 0; i < ProfileData::FilterSize; ++i)
			{
				if (profileData.TimeSamples[i] <= 0.0)
					continue;
				maxTime = Math::Max(profileData.TimeSamples[i], maxTime);
				avgTime += profileData.TimeSamples[i];
				++avgTimeSamples;
			}

			if (avgTimeSamples > 0)
				avgTime /= double(avgTimeSamples);

			if (profileData.Name == "Render")
				m_AvgRenderTime = avgTime;

			if (UI::Globals::bShowProfiler)
				orderedData.emplace(profileData.Name.c_str(), avgTime);
		}

		if (UI::Globals::bShowProfiler)
		{
			for (const OrderedData& data : orderedData)
				ImGui::Text("%s: %.2fms", data.Name, data.Time);

			ImGui::End();
		}
	}
}
