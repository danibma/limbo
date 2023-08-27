#include "stdafx.h"
#include "tests.h"

#if ENABLE_LIMBO_TESTS
#include <catch/catch.hpp>

#include "core/core.h"
#include "gfx/rhi/resourcepool.h"

struct Child
{
	uint32 i = 0;

	Child() = default;
	Child(uint32 x) : i(x) {}
};

bool operator==(const Child& left, const Child& right)
{
	return left.i == right.i;
}

TEST_CASE("resource_handles - allocate handle") 
{
	limbo::Gfx::Pool<Child, 5> pool;
	limbo::Gfx::Handle<Child> handle1 = pool.AllocateHandle();
	limbo::Gfx::Handle<Child> handle2 = pool.AllocateHandle();
	REQUIRE(handle1.IsValid());
	REQUIRE(handle2.IsValid());
}

TEST_CASE("resource_handles - get handle")
{
	Child c1 = Child(1);
	Child c2 = Child(2);
	limbo::Gfx::Pool<Child, 5> pool;
	limbo::Gfx::Handle<Child> handle1 = pool.AllocateHandle(c1);
	limbo::Gfx::Handle<Child> handle2 = pool.AllocateHandle(c2);
	Child* cc1 = pool.Get(handle1);
	Child* cc2 = pool.Get(handle2);
	REQUIRE(c1 == *cc1);
	REQUIRE(c2 == *cc2);
}

TEST_CASE("resource_handles - delete handle")
{
	Child c1 = Child(1);

	limbo::Gfx::Pool<Child, 5> pool;
	limbo::Gfx::Handle<Child> handle1 = pool.AllocateHandle(c1);
	Child* cc1 = pool.Get(handle1);
	REQUIRE(c1 == *cc1);
	pool.DeleteHandle(handle1);
	Child* deleted = pool.Get(handle1);
	REQUIRE(deleted == nullptr);
}
#endif