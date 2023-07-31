#pragma once

#include "core/core.h"

#define ENABLE_LIMBO_TESTS 0
#define LIMBO_TESTS_WARNING LB_ERROR("To run the tests, please set ENABLE_LIMBO_TESTS to 1 in tests.h"); return 0;

namespace limbo::Tests
{
	int ExecuteTests(char* args);
	int ExecuteComputeTriangle();
	int ExecuteGraphicsTriangle();
}
