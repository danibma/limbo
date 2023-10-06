#include "stdafx.h"
#include "tests.h"
#include "core/refcountptr.h"

using namespace limbo;

class Base1 : public RefCountedObject
{

};

#if ENABLE_LIMBO_TESTS
#include <catch/catch.hpp>

TEST_CASE("RefCountPtr - Creation/Copy/Deletion")
{
	LB_LOG("RefCountPtr - Creation/Copy/Deletion");
	RefCountPtr<Base1>* ptr;
	{
		RefCountPtr<Base1> b1(new Base1());
		ptr = &b1;
		{
			RefCountPtr<Base1> copyB1(b1);
			REQUIRE(b1->GetRefCount() == 2);
			REQUIRE(copyB1->GetRefCount() == 2);
		}
		REQUIRE(b1->GetRefCount() == 1);
	}
	REQUIRE_FALSE((*ptr).IsValid());
}

TEST_CASE("RefCountPtr - Move")
{
	LB_LOG("RefCountPtr - Move");

	LB_LOG("Constructor");
	{
		RefCountPtr<Base1> b1(new Base1());
		{
			RefCountPtr<Base1> copyB1(std::move(b1));
			REQUIRE_FALSE(b1.IsValid());
			REQUIRE(copyB1->GetRefCount() == 1);
		}
	}

	LB_LOG("Assignment operator");
	{
		RefCountPtr<Base1> b1(new Base1());
		{
			RefCountPtr<Base1> copyB1;
			copyB1 = std::move(b1);
			REQUIRE_FALSE(b1.IsValid());
			REQUIRE(copyB1->GetRefCount() == 1);
		}
	}
}

TEST_CASE("RefCountPtr - Swap")
{
	LB_LOG("RefCountPtr - Swap");
	RefCountPtr<Base1> b1(new Base1());
	RefCountPtr<Base1> copyB1;
	b1.Swap(copyB1);
	REQUIRE_FALSE(b1.IsValid());
	REQUIRE(copyB1->GetRefCount() == 1);
}

#endif