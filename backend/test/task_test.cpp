#include <catch2/catch_all.hpp>
#include "server.hpp"

#include <source_location>

template<typename T, typename U>
void check_expected_value(const std::expected<T, U>& expected, const T& value, std::source_location location = std::source_location::current())
{
	INFO("Location: " << location.file_name() << ":" << location.line());

	if (!expected.has_value())
	{
		UNSCOPED_INFO("std::expected error was: " << expected.error());
	}
	REQUIRE(expected.has_value());
	CHECK(expected.value() == value);
}

template<typename U, typename T>
void check_expected_error(const std::expected<T, U>& expected, const U& error, std::source_location location = std::source_location::current())
{
	INFO("Location: " << location.file_name() << ":" << location.line());

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

		SECTION("start another task")
		{
			REQUIRE(app.create_task("next task").has_value());

			const auto start_result_2 = app.start_task(TaskID(2));

			CHECK(!start_result_2.has_value());

			const auto state_result_1 = app.task_state(TaskID(1));
			const auto state_result_2 = app.task_state(TaskID(2));

			check_expected_value(state_result_1, TaskState::INACTIVE);
			check_expected_value(state_result_2, TaskState::ACTIVE);
		}
	}
}

TEST_CASE("Handling active task", "[task]")
{
	TestClock clock;
	std::ostringstream output;
	clock.time += std::chrono::hours(2);

	MicroTask app(clock, output);

	app.create_task("test 1");
	app.create_task("test 2");
	app.create_task("test 3");

	app.start_task(TaskID(1));

	auto state1 = app.task_state(TaskID(1));

	check_expected_value(state1, TaskState::ACTIVE);

	SECTION("Starting another task stops active task")
	{
		app.start_task(TaskID(2));

		state1 = app.task_state(TaskID(1));
		auto state2 = app.task_state(TaskID(2));

		check_expected_value(state1, TaskState::INACTIVE);
		check_expected_value(state2, TaskState::ACTIVE);
	}

	SECTION("Stopping active task clears active task")
	{
		app.stop_task(TaskID(1));

		app.finish_task(TaskID(1)); // finish the task to force it out of INACTIVE state

		app.start_task(TaskID(2));

		state1 = app.task_state(TaskID(1));
		auto state2 = app.task_state(TaskID(2));

		check_expected_value(state1, TaskState::FINISHED);
		check_expected_value(state2, TaskState::ACTIVE);
	}

	SECTION("Finishing active task clears active task")
	{
		app.finish_task(TaskID(1));

		app.start_task(TaskID(2));

		state1 = app.task_state(TaskID(1));
		auto state2 = app.task_state(TaskID(2));

		check_expected_value(state1, TaskState::FINISHED);
		check_expected_value(state2, TaskState::ACTIVE);
	}

	SECTION("Don't clear the active task if it isn't the task being finished")
	{
		app.finish_task(TaskID(3));
		app.start_task(TaskID(2));

		state1 = app.task_state(TaskID(1));
		auto state2 = app.task_state(TaskID(2));

		check_expected_value(state1, TaskState::INACTIVE);
		check_expected_value(state2, TaskState::ACTIVE);
	}
}
