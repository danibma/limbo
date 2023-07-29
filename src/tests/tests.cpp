#include "stdafx.h"
#include "tests.h"

#if ENABLE_LIMBO_TESTS
#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_DISABLE_EXCEPTIONS
#include <catch/catch.hpp>

#include "gfx/computetriangle.h"
#include "gfx/graphicstriangle.h"
#endif

namespace limbo::tests
{
	int executeTests(int argc, char* argv[])
	{
#if ENABLE_LIMBO_TESTS
		Catch::Session session;
		session.configData().waitForKeypress = Catch::WaitForKeypress::BeforeExit;

		const char* args[4];
		args[0] = argv[0];
		args[1] = "-r compact";
		args[2] = "--break";
		args[3] = "--success";
		return session.run(4, args);
#else
		LIMBO_TESTS_WARNING
#endif
	}

	int executeComputeTriangle()
	{
#if ENABLE_LIMBO_TESTS
		return gfx::runComputeTriangle();
#else
		LIMBO_TESTS_WARNING
#endif
	}

	int executeGraphicsTriangle()
	{
#if ENABLE_LIMBO_TESTS
		return gfx::runGfxTriangle();
#else
		LIMBO_TESTS_WARNING
#endif
	}
}