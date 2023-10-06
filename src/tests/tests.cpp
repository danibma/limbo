#include "stdafx.h"
#include "tests.h"

#if ENABLE_LIMBO_TESTS
#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_DISABLE_EXCEPTIONS
#include <catch/catch.hpp>
#endif

namespace limbo::Tests
{
	int ExecuteTests(char* args)
	{
#if ENABLE_LIMBO_TESTS
		Catch::Session session;
		session.configData().waitForKeypress = Catch::WaitForKeypress::BeforeExit;

		const char* arr_args[4];
		arr_args[0] = args;
		arr_args[1] = "-r compact";
		arr_args[2] = "--break";
		arr_args[3] = "--success";
		return session.run(4, arr_args);
#else
		LIMBO_TESTS_WARNING
#endif
	}
}