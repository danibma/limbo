#include <catch/catch.hpp>

#include <limbo.h>

struct Base
{
	uint32 i = 0;
};

struct Child : public Base
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
	limbo::Pool<Child, Base> pool;
	limbo::Handle<Base> handle1 = pool.allocateHandle();
	limbo::Handle<Base> handle2 = pool.allocateHandle();
	REQUIRE(handle1.isValid());
	REQUIRE(handle2.isValid());
}

TEST_CASE("resource_handles - get handle")
{
	Child c1 = Child(1);
	Child c2 = Child(2);
	limbo::Pool<Child, Base> pool;
	limbo::Handle<Base> handle1 = pool.allocateHandle(c1);
	limbo::Handle<Base> handle2 = pool.allocateHandle(c2);
	Child* cc1 = pool.get(handle1);
	Child* cc2 = pool.get(handle2);
	REQUIRE(c1 == *cc1);
	REQUIRE(c2 == *cc2);
}

TEST_CASE("resource_handles - delete handle")
{
	Child c1 = Child(1);

	limbo::Pool<Child, Base> pool;
	limbo::Handle<Base> handle1 = pool.allocateHandle(c1);
	Child* cc1 = pool.get(handle1);
	REQUIRE(c1 == *cc1);
	pool.deleteHandle(handle1);
	Child* deleted = pool.get(handle1);
	REQUIRE(deleted == nullptr);
}