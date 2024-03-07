#include <catch2/catch_all.hpp>
#include "lib.h"

TEST_CASE("hello world", "[first]")
{
	CHECK(true);
}

TEST_CASE("create new list", "[cpp]")
{
	MicroTask mt;

	const auto result = mt.create_list("test", 0);

	REQUIRE(result.has_value());
	CHECK(result.value() == 1);
}

TEST_CASE("create new list in non-root group", "[cpp]")
{
	MicroTask mt;

	REQUIRE(mt.create_group("nested", 0).has_value());

	const auto result = mt.create_list("test", 1);

	REQUIRE(result.has_value());
	CHECK(result.value() == 1);
}

TEST_CASE("create new list fails when group doesn't exist", "[failure]")
{
	MicroTask mt;

	const auto result = mt.create_list("test", 1);

	REQUIRE(!result.has_value());
	CHECK(result.error() == "Group with ID 1 does not exist.");
}

TEST_CASE("create new group", "[cpp]")
{
	MicroTask mt;

	const auto result = mt.create_group("test", 0);

	REQUIRE(result.has_value());
	CHECK(result.value() == 1);
}

// create new task

