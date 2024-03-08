#include <catch2/catch_all.hpp>
#include "lib.h"

template<typename T, typename U>
void check_expected_value(const std::expected<T, U>& expected, const T& value)
{
	if (!expected.has_value())
	{
		UNSCOPED_INFO("std::expected error was: " << expected.error());
	}
	REQUIRE(expected.has_value());
	CHECK(expected.value() == value);
}

template<typename U, typename T>
void check_expected_error(const std::expected<T, U>& expected, const U& error)
{
	REQUIRE(!expected.has_value());
	CHECK(expected.error() == error);
}


TEST_CASE("root group is ID 0", "[group]")
{
	CHECK(ROOT_GROUP_ID == 0);
}

TEST_CASE("create list", "[list]")
{
	MicroTask app;

	SECTION("create new list in root group")
	{
		const auto result = app.create_list("test", ROOT_GROUP_ID);

		check_expected_value(result, 1);
	}

	SECTION("create new list in non-root group")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());

		const auto result = app.create_list("test", 1);

		check_expected_value(result, 1);
	}

	SECTION("using existing list name in another group is ok")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());
		REQUIRE(app.create_list("test", ROOT_GROUP_ID).has_value());

		const auto result = app.create_list("test", 1);

		check_expected_value(result, 2);
	}

	SECTION("failure states")
	{
		SECTION("create new list fails when target group doesn't exist")
		{
			const auto result = app.create_list("test", 1);

			check_expected_error(result, std::string("Group with ID 1 does not exist."));
		}

		SECTION("create new list fails when list already exists in root group")
		{
			REQUIRE(app.create_list("test", ROOT_GROUP_ID).has_value());

			const auto result = app.create_list("test", ROOT_GROUP_ID);

			// TODO we should create facilities to format group and list names properly
			check_expected_error(result, std::string("List with name 'test' already exists in group with ID 0."));
		}

		SECTION("create new list fails when list already exists in non-root group")
		{
			REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());
			REQUIRE(app.create_list("test", 1).has_value());

			const auto result = app.create_list("test", 1);

			// TODO we should create facilities to format group and list names properly
			check_expected_error(result, std::string("List with name 'test' already exists in group with ID 1."));
		}
	}
}

TEST_CASE("create group", "[group]")
{
	MicroTask app;

	SECTION("create new group in root group")
	{
		const auto result = app.create_group("test", ROOT_GROUP_ID);

		check_expected_value(result, 1);
	}

	SECTION("create new group in non-root group")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());

		const auto result = app.create_group("test", 1);

		check_expected_value(result, 2);
	}

	SECTION("using existing group name in another group is ok")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());

		const auto result = app.create_group("nested", 1);

		check_expected_value(result, 2);
	}

	SECTION("create new group fails when group with name already exists in root group")
	{
		REQUIRE(app.create_group("test", ROOT_GROUP_ID).has_value());

		const auto result = app.create_group("test", ROOT_GROUP_ID);

		check_expected_error(result, std::string("Group with name 'test' already exists in group with ID 0."));
	}

	SECTION("create new group fails when group with name already exists in non-root group")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());
		REQUIRE(app.create_group("test", 1).has_value());

		const auto result = app.create_group("test", 1);

		check_expected_error(result, std::string("Group with name 'test' already exists in group with ID 1."));
	}

	SECTION("create new group fails when target group doesn't exist")
	{
		const auto result = app.create_group("test", 1);

		check_expected_error(result, std::string("Group with ID 1 does not exist."));
	}
}

// TODO test that creating a list in a finished group fails

// TODO test that creating a group in a finished group fails
// 
// create new task

