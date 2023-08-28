﻿#pragma once

#include <functional>

// Based of Wicked Engine's jobsystem by János Turánszki - https://wickedengine.net/2018/11/24/simple-job-system-using-standard-c/

namespace limbo::Core
{
	// A *ExecuteMany* job will receive this as function argument:
	struct JobDispatchArgs
	{
		uint32_t jobIndex;
		uint32_t groupIndex;
	};

	struct JobSystem
	{
        static void Initialize();

        static void Execute(const std::function<void()>& job);

        // Divide a job onto multiple jobs and execute in parallel.
        //  jobCount    : how many jobs to generate for this task.
        //  groupSize   : how many jobs to execute per thread. Jobs inside a group execute serially. It might be worth to increase for small jobs
        //  func        : receives a JobDispatchArgs as parameter
        static void ExecuteMany(uint32 jobCount, uint32 groupSize, const std::function<void(JobDispatchArgs)>& job);

        static bool IsBusy();

        static void WaitIdle();

		static uint32 ThreadCount();

	private:
		// Idle the main thread and wait until a worker thread is free
		static void WaitUntilFree();
	};
}