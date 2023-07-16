#include "tests.h"

#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_DISABLE_EXCEPTIONS
#include <catch/catch.hpp>

namespace limbo::tests
{
	int executeTests(int argc, char* argv[])
	{
		Catch::Session session;
		session.configData().waitForKeypress = Catch::WaitForKeypress::BeforeExit;

		const char* args[4];
		args[0] = argv[0];
		args[1] = "-r compact";
		args[2] = "--break";
		args[3] = "--success";
		return session.run(4, args);
	}
}