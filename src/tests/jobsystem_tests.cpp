#include "stdafx.h"
#include "tests.h"
#include "core/jobsystem.h"
#include "core/timer.h"

#if ENABLE_LIMBO_TESTS
#include <catch/catch.hpp>

TEST_CASE("jobsystem - Execute() speed")
{
    using namespace limbo;

    auto Spin = [](float milliseconds)
    {
        milliseconds /= 1000.0f;
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
        double ms = 0;
        while (ms < milliseconds)
        {
            std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
            ms = time_span.count();
        }
    };

    // Serial test
    {
        limbo::Core::Timer t;
        Spin(100);
        Spin(100);
        Spin(100);
        Spin(100);
        LB_LOG("Spins executed in: %.3fms", t.ElapsedMilliseconds());
    }

    // Execute test
    {
        limbo::Core::Timer t;
        Core::JobSystem::Execute(Core::TOnJobSystemExecute::CreateLambda([Spin] { Spin(100); }));
        Core::JobSystem::Execute(Core::TOnJobSystemExecute::CreateLambda([Spin] { Spin(100); }));
        Core::JobSystem::Execute(Core::TOnJobSystemExecute::CreateLambda([Spin] { Spin(100); }));
        Core::JobSystem::Execute(Core::TOnJobSystemExecute::CreateLambda([Spin] { Spin(100); }));
        Core::JobSystem::WaitIdle();
        LB_LOG("MT Spins executed in: %.3fms", t.ElapsedMilliseconds());
    }
}

TEST_CASE("jobsystem - ExecuteMany() speed")
{
    using namespace limbo;

    struct Data
    {
        float m[16];
        void Compute(uint32_t value)
        {
            for (int i = 0; i < 16; ++i)
            {
                m[i] += float(value + i);
            }
        }
    };
    constexpr uint32_t dataCount = 1'000'000;

    {
		Data* dataSet = new Data[dataCount];
        limbo::Core::Timer t;

        for (uint32_t i = 0; i < dataCount; ++i)
        {
            dataSet[i].Compute(i);
        }

        LB_LOG("Normal Loop executed in: %.3fms", t.ElapsedMilliseconds());
		delete[] dataSet;
    }

    // MT Loop
    {
        Data* dataSet = new Data[dataCount];
        limbo::Core::Timer t;

        constexpr uint32 groupSize = 10000;

        Core::JobSystem::ExecuteMany(dataCount, groupSize, Core::TOnJobSystemExecuteMany::CreateLambda([dataSet](Core::JobDispatchArgs args)
        {
            dataSet[args.jobIndex].Compute(args.jobIndex);
        }));
        Core::JobSystem::WaitIdle();

        LB_LOG("ExecuteMany() with %d jobs per thread executed in: %.3fms", groupSize, t.ElapsedMilliseconds());
        delete[] dataSet;
    }
}

#endif