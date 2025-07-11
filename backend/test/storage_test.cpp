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

	auto task = Task("this is a test", TaskID(1), NO_PARENT, std::chrono::milliseconds(0));
	REQUIRE(helper.database.tasks_written.size() == 1);
	CHECK(task == helper.database.tasks_written[0]);
}

TEST_CASE("Storage for Start Task", "[storage]")
{

}

TEST_CASE("Storage for Stop Task", "[storage]")
{

}

TEST_CASE("Storage for Finish Task", "[storage]")
{

}
