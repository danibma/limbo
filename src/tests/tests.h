#pragma once

#include "core/core.h"

#define ENABLE_LIMBO_TESTS 0
#define LIMBO_TESTS_WARNING LB_ERROR("To run the tests, please set ENABLE_LIMBO_TESTS to 1 in tests.h"); return 0;

namespace limbo::tests
{
	int executeTests(int argc, char* argv[]);
	int executeComputeTriangle();
	int executeGraphicsTriangle();
}
