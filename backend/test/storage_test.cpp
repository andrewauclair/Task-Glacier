#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "packets.hpp"

#include "utils.h"

// tests dealing with loading and saving data
// these tests will use a fake of the database to make sure we're calling it when we should



/*


create task
update task (reparent, lock, etc)
start task
stop task
finish task

bugzilla config
bugzilla refresh

time configuration modify/remove














*/

TEST_CASE("Storage for Create Task", "[storage]")
{
	TestHelper helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));

	auto task = Task("this is a test", TaskID(1), NO_PARENT, std::chrono::milliseconds(1737344039870));
	REQUIRE(helper.database.tasks_written.size() == 1);
	CHECK(task == helper.database.tasks_written[0]);
}

TEST_CASE("Storage for Start Task", "[storage]")
{
	TestHelper helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));

	helper.database.tasks_written.clear();

	helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

	auto task = Task("this is a test", TaskID(1), NO_PARENT, std::chrono::milliseconds(1737344039870));
	task.state = TaskState::ACTIVE;
	task.m_times.emplace_back(std::chrono::milliseconds(1737345839870));

	REQUIRE(helper.database.tasks_written.size() == 1);
	CHECK(task == helper.database.tasks_written[0]);
}

TEST_CASE("Storage for Stop Task", "[storage]")
{
	TestHelper helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));
	helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

	helper.database.tasks_written.clear();

	helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
	
	auto task = Task("this is a test", TaskID(1), NO_PARENT, std::chrono::milliseconds(1737344039870));
	task.m_times.emplace_back(std::chrono::milliseconds(1737345839870), std::chrono::milliseconds(1737347639870));

	REQUIRE(helper.database.tasks_written.size() == 1);
	CHECK(task == helper.database.tasks_written[0]);
}

TEST_CASE("Storage for Finish Task", "[storage]")
{
	TestHelper helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));
	helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

	helper.database.tasks_written.clear();

	helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));

	auto task = Task("this is a test", TaskID(1), NO_PARENT, std::chrono::milliseconds(1737344039870));
	task.m_times.emplace_back(std::chrono::milliseconds(1737345839870), std::chrono::milliseconds(1737347639870));
	task.state = TaskState::FINISHED;
	task.m_finishTime = std::chrono::milliseconds(1737347639870);

	REQUIRE(helper.database.tasks_written.size() == 1);
	CHECK(task == helper.database.tasks_written[0]);
}
