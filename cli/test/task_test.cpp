#include <catch2/catch_all.hpp>
#include "lib.hpp"

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
	CHECK(ROOT_GROUP_ID == GroupID(0));
}

TEST_CASE("create task", "[task]")
{
	MicroTask app;

	SECTION("create task on new list")
	{
		REQUIRE(app.create_list("test", ROOT_GROUP_ID).has_value());

		const auto result = app.create_task("testing", ListID(1));

		check_expected_value(result, TaskID(1));
	}

	SECTION("create multiple tasks")
	{
		REQUIRE(app.create_list("test", ROOT_GROUP_ID).has_value());

		auto result = app.create_task("one", ListID(1));
		check_expected_value(result, TaskID(1));

		result = app.create_task("two", ListID(1));
		check_expected_value(result, TaskID(2));
		
		result = app.create_task("three", ListID(1));
		check_expected_value(result, TaskID(3));
		
		result = app.create_task("four", ListID(1));
		check_expected_value(result, TaskID(4));
	}

	SECTION("failure states")
	{
		SECTION("list does not exist")
		{
			const auto result = app.create_task("testing", ListID(1));

			check_expected_error(result, std::string("List with ID 1 does not exist."));
		}

		SECTION("list is finished")
		{

		}
	}
}

TEST_CASE("create list", "[list]")
{
	MicroTask app;

	SECTION("create new list in root group")
	{
		const auto result = app.create_list("test", ROOT_GROUP_ID);

		check_expected_value(result, ListID(1));
	}

	SECTION("create new list in non-root group")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());

		const auto result = app.create_list("test", GroupID(1));

		check_expected_value(result, ListID(1));
	}

	SECTION("using existing list name in another group is ok")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());
		REQUIRE(app.create_list("test", ROOT_GROUP_ID).has_value());

		const auto result = app.create_list("test", GroupID(1));

		check_expected_value(result, ListID(2));
	}

	SECTION("failure states")
	{
		SECTION("create new list fails when target group doesn't exist")
		{
			const auto result = app.create_list("test", GroupID(1));

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
			REQUIRE(app.create_list("test", GroupID(1)).has_value());

			const auto result = app.create_list("test", GroupID(1));

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

		check_expected_value(result, GroupID(1));
	}

	SECTION("create new group in non-root group")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());

		const auto result = app.create_group("test", GroupID(1));

		check_expected_value(result, GroupID(2));
	}

	SECTION("using existing group name in another group is ok")
	{
		REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());

		const auto result = app.create_group("nested", GroupID(1));

		check_expected_value(result, GroupID(2));
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
		REQUIRE(app.create_group("test", GroupID(1)).has_value());

		const auto result = app.create_group("test", GroupID(1));

		check_expected_error(result, std::string("Group with name 'test' already exists in group with ID 1."));
	}

	SECTION("create new group fails when target group doesn't exist")
	{
		const auto result = app.create_group("test", GroupID(1));

		check_expected_error(result, std::string("Group with ID 1 does not exist."));
	}
}

//TEST_CASE("move list", "[list]")
//{
//	MicroTask app;
//	REQUIRE(app.create_list("test", ROOT_GROUP_ID).has_value());
//	REQUIRE(app.create_group("nested", ROOT_GROUP_ID).has_value());
//
//	CHECK(!app.move_list(1, 1).has_value());
//}

// TODO test that creating a list in a finished group fails

// TODO test that creating a group in a finished group fails
// 

TEST_CASE("task management", "[task]")
{
	MicroTask app;

	REQUIRE(app.create_list("test", ROOT_GROUP_ID).has_value());

	REQUIRE(app.create_task("testing", ListID(1)).has_value());

	SECTION("start a task")
	{
		const auto start_result = app.start_task(TaskID(1));

		CHECK(!start_result.has_value());

		const auto state_result = app.task_state(TaskID(1));

		check_expected_value(state_result, TaskState::ACTIVE);

		SECTION("stop a task")
		{
			const auto stop_result = app.stop_task(TaskID(1));

			CHECK(!stop_result.has_value());

			const auto state_result = app.task_state(TaskID(1));

			check_expected_value(state_result, TaskState::INACTIVE);
		}

		SECTION("finish a task")
		{
			const auto finish_result = app.finish_task(TaskID(1));

			CHECK(!finish_result.has_value());

			const auto state_result = app.task_state(TaskID(1));

			check_expected_value(state_result, TaskState::FINISHED);
		}
	}

	SECTION("starting task that doesn't exist fails")
	{
		const auto start_result = app.start_task(TaskID(2));

		REQUIRE(start_result.has_value());

		CHECK(start_result.value() == "Task with ID 2 does not exist.");
	}
}

TEST_CASE("Performance", "[.perf]")
{
	BENCHMARK("simple") {
		MicroTask app;
		app.create_list("testing", ROOT_GROUP_ID);

		for (int i = 0; i < 100'000; i++) {
			app.create_task("testing longer names for tasks", ListID(1));
		}
	};
}