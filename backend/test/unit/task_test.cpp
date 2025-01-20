#include <catch2/catch_all.hpp>
#include "server.hpp"

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


TEST_CASE("no parent ID is 0", "[task]")
{
	CHECK(NO_PARENT == TaskID(0));
}

TEST_CASE("create task", "[task]")
{
	TestClock clock;
	std::ostringstream output;
	MicroTask app(clock, output);

	SECTION("create task with no parent")
	{
		const auto result = app.create_task("testing");

		check_expected_value(result, TaskID(1));
	}

	SECTION("create multiple tasks")
	{
		auto result = app.create_task("one");
		check_expected_value(result, TaskID(1));

		result = app.create_task("two");
		check_expected_value(result, TaskID(2));
		
		result = app.create_task("three");
		check_expected_value(result, TaskID(3));
		
		result = app.create_task("four");
		check_expected_value(result, TaskID(4));
	}

	SECTION("failure states")
	{
		SECTION("Parent does not exist")
		{
			const auto result = app.create_task("testing", TaskID(1));

			check_expected_error(result, std::string("Task with ID 1 does not exist."));
		}

		SECTION("Parent is finished")
		{

		}
	}
}

TEST_CASE("task management", "[task]")
{
	TestClock clock;
	std::ostringstream output;
	clock.time += std::chrono::hours(2);

	MicroTask app(clock, output);

	REQUIRE(app.create_task("testing").has_value());

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
