#include "stdafx.h"
#include "jobsystem.h"
#include "ringbuffer.h"

#include <atomic>
#include <thread>
#include <condition_variable>

namespace limbo::Core
{
    uint32                                  SNumThreads = 0;
    RingBuffer<std::function<void()>, 256>  SJobPool;
    std::condition_variable                 SWakeCondition;
    std::mutex                              SWakeMutex;
    uint64                                  SCurrentValue = 0;
    std::atomic<uint64>                     SCompletedValue;


    void JobSystem::Initialize()
    {
        SCompletedValue.store(0);
        SNumThreads = Math::Max(1u, std::thread::hardware_concurrency());

        // Create all our worker threads while immediately starting them:
        for (uint32 threadID = 0; threadID < SNumThreads; ++threadID)
        {
            std::thread worker([]()
            {
                std::function<void()> job;

                // This is the infinite loop that a worker thread will do 
                while (true)
                {
                    if (SJobPool.PopFront(job)) // try to grab a job from the jobPool queue
                    {
                        job();
                        SCompletedValue.fetch_add(1);
                    }
                    else
                    {
                        // no job, put thread to sleep
                        std::unique_lock<std::mutex> lock(SWakeMutex);
                        SWakeCondition.wait(lock);
                    }
                }
            });

            HANDLE handle = (HANDLE)worker.native_handle();

            // Put each thread onto the core specified by the threadID
            //DWORD_PTR affinityMask    = 1ull << threadID;
            //DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
            //check(affinity_result > 0);

            std::wstring threadName = std::format(L"Worker Thread {}", threadID);
            HRESULT hr = SetThreadDescription(handle, threadName.c_str());
            check(SUCCEEDED(hr));

            worker.detach();
        }
    }

    void JobSystem::Execute(const std::function<void()>& job)
    {
        SCurrentValue += 1;

        // Try to push a new job until it is pushed successfully
        while (!SJobPool.PushBack(job)) { WaitUntilFree(); }

        SWakeCondition.notify_one();
    }

    void JobSystem::ExecuteMany(uint32 jobCount, uint32 groupSize, const std::function<void(JobDispatchArgs)>& job)
    {
        if (jobCount == 0 || groupSize == 0)
            return;

        // Calculate the amount of job groups to dispatch (overestimate)
        const uint32 threadsToUse = (jobCount + groupSize - 1) / groupSize;

        SCurrentValue += threadsToUse;

        for (uint32 groupIndex = 0; groupIndex < threadsToUse; ++groupIndex)
        {
            // For each group, generate one real job
            auto jobGroup = [jobCount, groupSize, job, groupIndex]()
        	{
                // Calculate the current group's offset into the jobs
                const uint32 groupJobOffset = groupIndex * groupSize;
                const uint32 groupJobEnd = Math::Min(groupJobOffset + groupSize, jobCount);

                JobDispatchArgs args;
                args.groupIndex = groupIndex;

                // Inside the group, loop through all job indices and execute job for each index
                for (uint32 i = groupJobOffset; i < groupJobEnd; ++i)
                {
                    args.jobIndex = i;
                    job(args);
                }
            };

            // Try to push a new job until it is pushed successfully:
            while (!SJobPool.PushBack(jobGroup)) { WaitUntilFree(); }

            SWakeCondition.notify_one(); // wake one thread
        }
    }

    bool JobSystem::IsBusy()
    {
        return SCompletedValue.load() < SCurrentValue;
    }

    void JobSystem::WaitIdle()
    {
        while (IsBusy()) { WaitUntilFree(); }
    }

    uint32 JobSystem::ThreadCount()
    {
        return SNumThreads;
    }

    void JobSystem::WaitUntilFree()
    {
        SWakeCondition.notify_one();
        std::this_thread::yield(); // Reschedule the main thread
    }
}
