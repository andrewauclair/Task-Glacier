#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "server.hpp"
#include "packets.hpp"
#include "utils.h"

#include <vector>
#include <source_location>

TEST_CASE("no parent ID is 0", "[task]")
{
	CHECK(NO_PARENT == TaskID(0));
}

TEST_CASE("Create Task", "[api][task]")
{
	TestHelper helper;

	SECTION("Success")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "this is a test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::INACTIVE;
		taskInfo.newTask = true;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Create Task With Parent")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(TaskID(1), helper.next_request_id(), "test 2"));

		auto taskInfo = TaskInfoMessage(TaskID(2), TaskID(1), "test 2");

		taskInfo.createTime = std::chrono::milliseconds(1737345839870);
		taskInfo.state = TaskState::INACTIVE;
		taskInfo.newTask = true;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Create Task Time Entry")
	{
		auto create = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1");
		create.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::INACTIVE;
		taskInfo.newTask = true;

		taskInfo.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };
		
		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Parent Task Does Not Exist")
	{
		helper.expect_failure(CreateTaskMessage(TaskID(2), helper.next_request_id(), "this is a test"), "Task with ID 2 does not exist.");

		// verify that the task was not created
		helper.expect_failure(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(2)), "Task with ID 2 does not exist.");
	}

	SECTION("Failure - Parent Is Finished")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_failure(CreateTaskMessage(TaskID(1), helper.next_request_id(), "test 2"), "Cannot add sub-task. Task with ID 1 is finished.");

		// verify that the task was not created
		helper.expect_failure(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(2)), "Task with ID 2 does not exist.");
	}

	SECTION("Persist")
	{
		auto create = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test");
		create.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create);

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "create 1 0 1737344939870 0 (14 this is a test)\ntask-time-codes 1 1 2 2 3 \n");
	}
}

TEST_CASE("Start Task", "[api][task]")
{
	TestHelper helper;

	SECTION("Success")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.emplace_back(std::chrono::milliseconds(1737347639870));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Start Another Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));

		auto taskInfo1 = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");
		
		taskInfo1.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo1.times.emplace_back(std::chrono::milliseconds(1737347639870), std::chrono::milliseconds(1737349439870));
		taskInfo1.state = TaskState::INACTIVE;
		taskInfo1.newTask = false;

		auto taskInfo2 = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");

		taskInfo2.createTime = std::chrono::milliseconds(1737345839870);
		taskInfo2.times.emplace_back(std::chrono::milliseconds(1737350339870));
		taskInfo2.state = TaskState::ACTIVE;
		taskInfo2.newTask = false;

		helper.required_messages({ &taskInfo1, &taskInfo2 });
	}

	SECTION("Failure - Task Does Not Exist")
	{
		helper.expect_failure(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Task Is Already Active")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_failure(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 is already active.");

		// active task remains active and hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.emplace_back(std::chrono::milliseconds(1737345839870));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Task Is Finished")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_failure(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 is finished.");

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.finishTime = std::chrono::milliseconds(1737345839870);
		taskInfo.state = TaskState::FINISHED;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Persist - Without Time Entry")
	{
		auto create = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1");
		helper.expect_success(create);

		helper.clear_file_output();

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "start 1 1737346739870 0 \n");
	}

	SECTION("Persist - With Time Entry")
	{
		auto create = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1");
		create.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create);

		helper.clear_file_output();

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "start 1 1737346739870 1 2 2 3 \n");
	}
}

TEST_CASE("Start Task - Time Entry", "[api][task]")
{
	TestHelper helper;

	auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
	auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A", "A");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

	auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B", "B");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");

	helper.expect_success(modify);

	SECTION("Success")
	{
		CreateTaskMessage create(NO_PARENT, helper.next_request_id(), "test 1");
		create.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create);
		
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737345839870);
		times.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		taskInfo.times.push_back(times);
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;
		taskInfo.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.required_messages({ &taskInfo });
	}

	SECTION("Time codes on task take priority over parent")
	{
		CreateTaskMessage create1(NO_PARENT, helper.next_request_id(), "test 1");
		create1.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create1);

		CreateTaskMessage create2(TaskID(1), helper.next_request_id(), "test 2");
		create2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create2);

		helper.clear_message_output();

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));

		auto taskInfo = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");
		taskInfo.parentID = TaskID(1);
		taskInfo.createTime = std::chrono::milliseconds(1737345839870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737347639870);
		times.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		taskInfo.times.push_back(times);
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;
		taskInfo.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.required_messages({ &taskInfo });
	}

	SECTION("Inherit from parent if no time code is found for category")
	{
		CreateTaskMessage create1(NO_PARENT, helper.next_request_id(), "test 1");
		create1.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create1);

		CreateTaskMessage create2(TaskID(1), helper.next_request_id(), "test 2");

		helper.expect_success(create2);

		helper.clear_message_output();

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));

		auto taskInfo = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");
		taskInfo.parentID = TaskID(1);
		taskInfo.createTime = std::chrono::milliseconds(1737345839870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737347639870);
		times.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		taskInfo.times.push_back(times);
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;
		
		helper.required_messages({ &taskInfo });
	}

	SECTION("Move to next parent if parent has no time codes")
	{
		CreateTaskMessage create1(NO_PARENT, helper.next_request_id(), "test 1");
		create1.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create1);

		CreateTaskMessage create2(TaskID(1), helper.next_request_id(), "test 2");
		helper.expect_success(create2);

		CreateTaskMessage create3(TaskID(2), helper.next_request_id(), "test 3");
		helper.expect_success(create3);

		helper.clear_message_output();

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));

		auto taskInfo = TaskInfoMessage(TaskID(3), NO_PARENT, "test 3");
		taskInfo.parentID = TaskID(2);
		taskInfo.createTime = std::chrono::milliseconds(1737347639870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737349439870);
		times.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		taskInfo.times.push_back(times);
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Inherit Time Code from Time Category That Task Does Not Have a Time Code For")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		
		auto& newCategory3 = modify.timeCategories.emplace_back(TimeCategoryID(0), "C", "C");
		newCategory3.codes.emplace_back(TimeCodeID(0), "Code 5");
		newCategory3.codes.emplace_back(TimeCodeID(0), "Code 6");

		helper.expect_success(modify);

		CreateTaskMessage create1(NO_PARENT, helper.next_request_id(), "test 1");
		create1.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create1);

		CreateTaskMessage create2(TaskID(1), helper.next_request_id(), "test 2");
		helper.expect_success(create2);

		CreateTaskMessage create3(TaskID(2), helper.next_request_id(), "test 3");
		create3.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(5)} };

		helper.expect_success(create3);

		helper.clear_message_output();

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));

		auto taskInfo = TaskInfoMessage(TaskID(3), NO_PARENT, "test 3");
		taskInfo.parentID = TaskID(2);
		taskInfo.createTime = std::chrono::milliseconds(1737347639870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737349439870);
		times.timeEntry = std::vector{ TimeEntry{ TimeCategoryID(1), TimeCodeID(5) }, TimeEntry{ TimeCategoryID(2), TimeCodeID(3) }, TimeEntry{ TimeCategoryID(3), TimeCodeID(0) } };

		taskInfo.times.push_back(times);
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;
		taskInfo.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(5)} };

		helper.required_messages({ &taskInfo });
	}

	SECTION("Use no time codes if none of parents have time codes")
	{
		CreateTaskMessage create1(NO_PARENT, helper.next_request_id(), "test 1");

		helper.expect_success(create1);

		CreateTaskMessage create2(TaskID(1), helper.next_request_id(), "test 2");
		helper.expect_success(create2);

		CreateTaskMessage create3(TaskID(2), helper.next_request_id(), "test 3");
		helper.expect_success(create3);

		helper.clear_message_output();

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));

		auto taskInfo = TaskInfoMessage(TaskID(3), NO_PARENT, "test 3");
		taskInfo.parentID = TaskID(2);
		taskInfo.createTime = std::chrono::milliseconds(1737347639870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737349439870);
		times.timeEntry = std::vector{ TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, TimeEntry{ TimeCategoryID(2), TimeCodeID(0) } };

		taskInfo.times.push_back(times);
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}
}

TEST_CASE("Stop Task", "[api][task]")
{
	TestHelper helper;

	SECTION("Success - Stop Active Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.emplace_back(std::chrono::milliseconds(1737345839870), std::chrono::milliseconds(1737347639870));
		taskInfo.state = TaskState::INACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Stopping Active Task Clears Active Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1))); // sotp the task. this step is critical to the test
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1))); // finish the task to force it out of INACTIVE state

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));

		auto taskInfo = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");

		taskInfo.createTime = std::chrono::milliseconds(1737345839870);
		taskInfo.times.emplace_back(std::chrono::milliseconds(1737353039870));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.emplace_back(std::chrono::milliseconds(1737347639870), std::chrono::milliseconds(1737349439870));
		taskInfo.finishTime = std::chrono::milliseconds(1737351239870);
		taskInfo.state = TaskState::FINISHED;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Task Does Not Exist")
	{
		helper.expect_failure(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Task Is Not Active")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));

		helper.expect_failure(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 is not active.");

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::INACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Task Is Finished")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_failure(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 is finished.");

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.finishTime = std::chrono::milliseconds(1737345839870);
		taskInfo.state = TaskState::FINISHED;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Persist")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.clear_file_output();

		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "stop 1 1737348539870\n");
	}
}

TEST_CASE("Finish Task", "[api][task]")
{
	TestHelper helper;

	SECTION("Success - Finish Active Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.emplace_back(std::chrono::milliseconds(1737345839870), std::chrono::milliseconds(1737347639870));
		taskInfo.finishTime = std::chrono::milliseconds(1737347639870);
		taskInfo.state = TaskState::FINISHED;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Finish Task That Is Not Active")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));

		SECTION("With Active Task")
		{
			helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

			helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(2)));

			auto taskInfo = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");

			taskInfo.createTime = std::chrono::milliseconds(1737345839870);
			taskInfo.finishTime = std::chrono::milliseconds(1737349439870);
			taskInfo.state = TaskState::FINISHED;
			taskInfo.newTask = false;

			helper.required_messages({ &taskInfo });

			// active task remains active and hasn't been modified
			helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

			taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

			taskInfo.createTime = std::chrono::milliseconds(1737344039870);
			taskInfo.times.emplace_back(std::chrono::milliseconds(1737347639870));
			taskInfo.state = TaskState::ACTIVE;
			taskInfo.newTask = false;

			helper.required_messages({ &taskInfo });
		}

		SECTION("Without Active Task")
		{
			helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(2)));

			auto taskInfo = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");

			taskInfo.createTime = std::chrono::milliseconds(1737345839870);
			taskInfo.finishTime = std::chrono::milliseconds(1737347639870);
			taskInfo.state = TaskState::FINISHED;
			taskInfo.newTask = false;

			helper.required_messages({ &taskInfo });
		}
	}

	SECTION("Success - Finishing Active Task Clears Active Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));

		auto taskInfo = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");

		taskInfo.createTime = std::chrono::milliseconds(1737345839870);
		taskInfo.times.emplace_back(std::chrono::milliseconds(1737351239870));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.emplace_back(std::chrono::milliseconds(1737347639870), std::chrono::milliseconds(1737349439870));
		taskInfo.finishTime = std::chrono::milliseconds(1737349439870);
		taskInfo.state = TaskState::FINISHED;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Do Not Clear Active Task When Finished Another Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 3"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(2)));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));

		auto taskInfo1 = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo1.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo1.times.emplace_back(std::chrono::milliseconds(1737349439870), std::chrono::milliseconds(1737353039870));
		taskInfo1.state = TaskState::INACTIVE;
		taskInfo1.newTask = false;

		auto taskInfo3 = TaskInfoMessage(TaskID(3), NO_PARENT, "test 3");

		taskInfo3.createTime = std::chrono::milliseconds(1737347639870);
		taskInfo3.times.emplace_back(std::chrono::milliseconds(1737353939870));
		taskInfo3.state = TaskState::ACTIVE;
		taskInfo3.newTask = false;

		helper.required_messages({ &taskInfo1, &taskInfo3 });

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		helper.required_messages({ &taskInfo1 });
	}

	SECTION("Failure - Task Does Not Exist")
	{
		helper.expect_failure(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Task Is Already Finished")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_failure(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 is already finished.");
	}

	// TODO we'll have some special cases about not finishing parents before children

	SECTION("Persist")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.clear_file_output();

		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "finish 1 1737348539870\n");
	}
}

TEST_CASE("Modify Task", "[api][task]")
{
	TestHelper helper;

	SECTION("Success - Rename Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(UpdateTaskMessage(helper.next_request_id(), TaskID(1), NO_PARENT, "something else"));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "something else");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::INACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Reparent Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(UpdateTaskMessage(helper.next_request_id(), TaskID(1), TaskID(2), "test"));

		auto taskInfo = TaskInfoMessage(TaskID(1), TaskID(2), "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::INACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Task Does Not Exist")
	{
		helper.expect_failure(UpdateTaskMessage(helper.next_request_id(), TaskID(1), NO_PARENT, "test"), "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Parent Task Does Not Exist")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_failure(UpdateTaskMessage(helper.next_request_id(), TaskID(1), TaskID(2), "test"), "Task with ID 2 does not exist.");
	}

	SECTION("Persist - Rename Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		
		helper.clear_file_output();

		helper.expect_success(UpdateTaskMessage(helper.next_request_id(), TaskID(1), NO_PARENT, "something else"));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "rename 1 (14 something else)\n");
	}

	SECTION("Persist - Reparent Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));

		helper.clear_file_output();

		helper.expect_success(UpdateTaskMessage(helper.next_request_id(), TaskID(1), TaskID(2), "test"));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "reparent 1 2\n");
	}

	SECTION("Success - Change Task Time Codes")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));

		helper.clear_file_output();

		UpdateTaskMessage update(helper.next_request_id(), TaskID(1), NO_PARENT, "test");
		update.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		// TODO validate that time codes are valid
		helper.expect_success(update);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::INACTIVE;
		taskInfo.newTask = false;
		taskInfo.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.required_messages({ &taskInfo });
	}

	SECTION("Persist - Change Task Time Codes")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));

		helper.clear_file_output();

		UpdateTaskMessage update(helper.next_request_id(), TaskID(1), NO_PARENT, "test");
		update.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		// TODO validate that time codes are valid
		helper.expect_success(update);

		CHECK(helper.fileOutput.str() == "task-time-codes 1 1 2 2 3 \n");
	}
}

TEST_CASE("Time Categories and Time Codes", "[api][task]")
{
	// success 
	// - add time category
	// - add time code
	// - time code id increments
	// - time code id is not reused when deleting time code
	// - delete time category
	// - delete time code
	
	// failure
	// - time category already exists
	// - time category does not exist (when adding time code)
	// - time code already exists on time category
	// - time category cannot be deleted once in use
	// - time code cannot be deleted once in use
	// - time category can be archived if all associated tasks are finished
	// - time code can be archived if all associated tasks are finished
	// - task cannot be changed back to inactive from finished if using an archived time category or time code (I don't think this feature exists yet)
	TestHelper helper;

	SECTION("Success - Add Time Category")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		auto& newCategory = modify.timeCategories.emplace_back(TimeCategoryID(0), "New", "NEW");
		newCategory.codes.emplace_back(TimeCodeID(0), "Code 1");

		helper.expect_success(modify);

		auto data = TimeEntryDataPacket({});
		auto& verifyCategory = data.timeCategories.emplace_back(TimeCategoryID(1), "New", "NEW");
		verifyCategory.codes.emplace_back(TimeCodeID(1), "Code 1");

		helper.required_messages({ &data });
	}

	SECTION("Success - Add Time Code")
	{
		{
			auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
			auto& newCategory = modify.timeCategories.emplace_back(TimeCategoryID(0), "New");

			helper.expect_success(modify);
		}

		helper.clear_message_output();

		{
			auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
			auto& newCategory = modify.timeCategories.emplace_back(TimeCategoryID(1), "New");
			newCategory.codes.emplace_back(TimeCodeID(0), "Code 1");

			helper.expect_success(modify);
		}

		auto data = TimeEntryDataPacket({});
		auto& verifyCategory = data.timeCategories.emplace_back(TimeCategoryID(1), "New");
		verifyCategory.codes.emplace_back(TimeCodeID(1), "Code 1");

		helper.required_messages({ &data });
	}

	SECTION("Success - Add Multiple Time Categories")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		modify.timeCategories.emplace_back(TimeCategoryID(0), "New 1");
		modify.timeCategories.emplace_back(TimeCategoryID(0), "New 2");

		helper.expect_success(modify);

		auto data = TimeEntryDataPacket({});
		data.timeCategories.emplace_back(TimeCategoryID(1), "New 1");
		data.timeCategories.emplace_back(TimeCategoryID(2), "New 2");

		helper.required_messages({ &data });
	}

	SECTION("Success - Add Multiple Time Codes")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		
		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 2");

		auto& category2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 2");
		category2.codes.emplace_back(TimeCodeID(0), "Code 3");
		category2.codes.emplace_back(TimeCodeID(0), "Code 4");

		helper.expect_success(modify);

		auto data = TimeEntryDataPacket({});

		auto& verifyCategory1 = data.timeCategories.emplace_back(TimeCategoryID(1), "New 1");
		verifyCategory1.codes.emplace_back(TimeCodeID(1), "Code 1");
		verifyCategory1.codes.emplace_back(TimeCodeID(2), "Code 2");

		auto& verifyCategory2 = data.timeCategories.emplace_back(TimeCategoryID(2), "New 2");
		verifyCategory2.codes.emplace_back(TimeCodeID(3), "Code 3");
		verifyCategory2.codes.emplace_back(TimeCodeID(4), "Code 4");

		helper.required_messages({ &data });
	}

	SECTION("Success - Update Time Category Name")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 2");

		helper.expect_success(modify);
		helper.clear_message_output();

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		modify.timeCategories.emplace_back(TimeCategoryID(1), "New Name");

		helper.expect_success(modify);
	}

	SECTION("Success - Update Time Code Name")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 2");

		helper.expect_success(modify);
		helper.clear_message_output();

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		auto& cat1 = modify.timeCategories.emplace_back(TimeCategoryID(1), "New 1");
		cat1.codes.emplace_back(TimeCodeID(1), "Code One");
		cat1.codes.emplace_back(TimeCodeID(2), "Code One");

		helper.expect_success(modify);
	}

	SECTION("Success - Delete Time Category")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 2");

		auto& category2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 2");
		category2.codes.emplace_back(TimeCodeID(0), "Code 3");
		category2.codes.emplace_back(TimeCodeID(0), "Code 4");

		helper.expect_success(modify);

		helper.clear_message_output();

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::REMOVE_CATEGORY, {});

		modify.timeCategories.emplace_back(TimeCategoryID(2), "New 2");

		helper.expect_success(modify);

		auto data = TimeEntryDataPacket({});

		auto& verifyCategory1 = data.timeCategories.emplace_back(TimeCategoryID(1), "New 1");
		verifyCategory1.codes.emplace_back(TimeCodeID(1), "Code 1");
		verifyCategory1.codes.emplace_back(TimeCodeID(2), "Code 2");

		helper.required_messages({ &data });
	}

	SECTION("Success - Delete Time Code")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 2");

		auto& category2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 2");
		category2.codes.emplace_back(TimeCodeID(0), "Code 3");
		category2.codes.emplace_back(TimeCodeID(0), "Code 4");

		helper.expect_success(modify);

		helper.clear_message_output();

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::REMOVE_CODE, {});

		auto& toRemove = modify.timeCategories.emplace_back(TimeCategoryID(2), "New 2");
		toRemove.codes.emplace_back(TimeCodeID(4), "Code 4");

		helper.expect_success(modify);

		auto data = TimeEntryDataPacket({});

		auto& verifyCategory1 = data.timeCategories.emplace_back(TimeCategoryID(1), "New 1");
		verifyCategory1.codes.emplace_back(TimeCodeID(1), "Code 1");
		verifyCategory1.codes.emplace_back(TimeCodeID(2), "Code 2");

		auto& verifyCategory2 = data.timeCategories.emplace_back(TimeCategoryID(2), "New 2");
		verifyCategory2.codes.emplace_back(TimeCodeID(3), "Code 3");

		helper.required_messages({ &data });
	}

	SECTION("Success - Fine to Have Same Time Code in Multiple Time Categories")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 1");
		category1.codes.emplace_back(TimeCodeID(0), "Code 2");

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		helper.expect_success(modify);

		auto& category2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New 2");
		category2.codes.emplace_back(TimeCodeID(0), "Code 1");
		category2.codes.emplace_back(TimeCodeID(0), "Code 2");

		helper.expect_success(modify);
	}

	SECTION("Failure - Time Category Already Exists")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New");

		helper.expect_success(modify);
		helper.clear_message_output();

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		
		auto& category2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New");

		helper.expect_failure(modify, "Time Category with name 'New' already exists");
	}

	SECTION("Failure - Time Category Does Not Exist")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::UPDATE, {});
		modify.timeCategories.emplace_back(TimeCategoryID(1), "Update");
		
		helper.expect_failure(modify, "Time Category with ID 1 does not exist");
	}

	SECTION("Failure - Time Code Already Exists")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New");
		category1.codes.emplace_back(TimeCodeID(0), "Code 1");

		helper.expect_success(modify);
		helper.clear_message_output();

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category2 = modify.timeCategories.emplace_back(TimeCategoryID(1), "New");
		category2.codes.emplace_back(TimeCodeID(0), "Code 1");

		helper.expect_failure(modify, "Time Code with name 'Code 1' already exists on Time Category 'New'");
	}

	SECTION("Failure - Time Code Does Not Exist for Update")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New");

		helper.expect_success(modify);
		helper.clear_message_output();

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::UPDATE, {});
		auto& cat1 = modify.timeCategories.emplace_back(TimeCategoryID(1), "New");
		cat1.codes.emplace_back(TimeCodeID(1), "Code 1");

		helper.expect_failure(modify, "Time Code with ID 1 does not exist");
	}

	SECTION("Failure - Time Code Does Not Exist for Remove Code")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});

		auto& category1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "New");

		helper.expect_success(modify);
		helper.clear_message_output();

		modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::REMOVE_CODE, {});
		auto& cat1 = modify.timeCategories.emplace_back(TimeCategoryID(1), "New");
		cat1.codes.emplace_back(TimeCodeID(1), "Code 1");

		helper.expect_failure(modify, "Time Code with ID 1 does not exist");
	}
}

TEST_CASE("Request Daily Report", "[api][task]")
{
	TestHelper helper;

	auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
	auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A", "A");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

	auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B", "B");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 5");

	helper.expect_success(modify);

	helper.clear_message_output();

	SECTION("No Report Found")
	{
		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;

		helper.required_messages({ &report });
	}

	SECTION("Report Found")
	{
		helper.clock.time = date_to_ms(2, 3, 2025) + std::chrono::hours(5);

		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5) + std::chrono::minutes(30);
		report.report.times.emplace_back(TaskID(1), 0);
		report.report.totalTime = std::chrono::minutes(30);
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, std::chrono::minutes(30));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(0) }, std::chrono::minutes(30));

		helper.required_messages({ &report });
	}

	SECTION("Report Tasks For Previous Day")
	{
		helper.clock.time = date_to_ms(2, 3, 2025) + std::chrono::hours(5);

		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.clock.time += std::chrono::days(1);
		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5) + std::chrono::minutes(30);
		report.report.times.emplace_back(TaskID(1), 0);
		report.report.totalTime = std::chrono::milliseconds(88200000);
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, std::chrono::milliseconds(88200000));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(0) }, std::chrono::milliseconds(88200000));

		helper.required_messages({ &report });
	}

	SECTION("Only Report Tasks For Tasks on Day")
	{
		helper.clock.time = date_to_ms(2, 3, 2025);

		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 4, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.month = 2;
		report.report.day = 4;
		report.report.year = 2025;

		helper.required_messages({ &report });
	}

	SECTION("Start of Day")
	{
		helper.clock.time = date_to_ms(2, 3, 2025) + std::chrono::hours(5);

		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5) + std::chrono::minutes(30);
		report.report.times.emplace_back(TaskID(1), 0);
		report.report.totalTime = std::chrono::minutes(30);
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, std::chrono::minutes(30));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(0) }, std::chrono::minutes(30));

		helper.required_messages({ &report });
	}

	SECTION("End of Day")
	{
		helper.clock.auto_increment_test_time = false;
		helper.clock.time = date_to_ms(2, 3, 2025) + std::chrono::hours(5);

		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5);
		report.report.endTime = date_to_ms(2, 3, 2025) + std::chrono::hours(7);
		report.report.times.emplace_back(TaskID(1), 0);
		report.report.totalTime = std::chrono::hours(2);
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, std::chrono::hours(2));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(0) }, std::chrono::hours(2));
		helper.required_messages({ &report });
	}

	SECTION("Basic Time Pair")
	{
		helper.clock.auto_increment_test_time = false;
		helper.clock.time = date_to_ms(2, 3, 2025) + std::chrono::hours(5);

		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5);
		report.report.endTime = date_to_ms(2, 3, 2025) + std::chrono::hours(7);
		report.report.times.emplace_back(TaskID(1), 0);
		report.report.totalTime = std::chrono::hours(2);
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, std::chrono::hours(2));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(0) }, std::chrono::hours(2));
		helper.required_messages({ &report });
	}

	SECTION("Time Pair for Task on Multiple Days")
	{
		helper.clock.auto_increment_test_time = false;
		helper.clock.time = date_to_ms(2, 2, 2025) + std::chrono::hours(5);

		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		helper.clock.time = date_to_ms(2, 3, 2025) + std::chrono::hours(5);

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		helper.clock.time = date_to_ms(2, 4, 2025) + std::chrono::hours(5);

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5);
		report.report.endTime = date_to_ms(2, 3, 2025) + std::chrono::hours(7);
		report.report.times.emplace_back(TaskID(1), 1);
		report.report.totalTime = std::chrono::hours(2);
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, std::chrono::hours(2));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(0) }, std::chrono::hours(2));
		helper.required_messages({ &report });
	}

	SECTION("Totals Per Time Entry")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "C", "C");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 6");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 7");

		auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "D", "D");
		newCategory2.codes.emplace_back(TimeCodeID(0), "Code 8");
		newCategory2.codes.emplace_back(TimeCodeID(0), "Code 9");

		helper.expect_success(modify);

		helper.clear_message_output();

		// two tasks that happen on the same day twice, but also happen on other days
		helper.clock.auto_increment_test_time = false;
		helper.clock.time = date_to_ms(2, 2, 2025) + std::chrono::hours(5);

		auto create1 = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1");
		auto create2 = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2");
		auto create3 = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 3");

		create1.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };
		create2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(3), TimeCodeID(4)}, TimeEntry{TimeCategoryID(4), TimeCodeID(5)} };

		helper.expect_success(create1);
		helper.expect_success(create2);
		helper.expect_success(create3);
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(2)));

		helper.clock.time = date_to_ms(2, 3, 2025) + std::chrono::hours(5);

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(3);
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(3)));

		helper.clock.time = date_to_ms(2, 4, 2025) + std::chrono::hours(5);

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));
		helper.clock.time += std::chrono::hours(2);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(2)));

		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5);
		report.report.endTime = date_to_ms(2, 3, 2025) + std::chrono::hours(14);
		report.report.times.emplace_back(TaskID(1), 1);
		report.report.times.emplace_back(TaskID(1), 2);
		report.report.times.emplace_back(TaskID(2), 1);
		report.report.times.emplace_back(TaskID(3), 0);
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, std::chrono::hours(4));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(2) }, std::chrono::hours(5));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(0) }, std::chrono::hours(4));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(3) }, std::chrono::hours(5));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(3), TimeCodeID(0) }, std::chrono::hours(7));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(3), TimeCodeID(4) }, std::chrono::hours(2));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(4), TimeCodeID(0) }, std::chrono::hours(7));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(4), TimeCodeID(5) }, std::chrono::hours(2));
		report.report.totalTime = std::chrono::hours(9);

		helper.required_messages({ &report });
	}

	SECTION("Inherited Time Entry")
	{
		// two tasks that happen on the same day twice, but also happen on other days
		helper.clock.auto_increment_test_time = false;
		helper.clock.time = date_to_ms(2, 2, 2025) + std::chrono::hours(5);

		auto create1 = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1");
		auto create2 = CreateTaskMessage(TaskID(1), helper.next_request_id(), "test 2");
		auto create3 = CreateTaskMessage(TaskID(2), helper.next_request_id(), "test 3");

		create1.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create1);
		helper.expect_success(create2);
		helper.expect_success(create3);

		helper.clock.time = date_to_ms(2, 3, 2025) + std::chrono::hours(5);

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));
		helper.clock.time += std::chrono::hours(3);
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(3)));

		helper.clear_message_output();

		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = DailyReportMessage(helper.prev_request_id());
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5);
		report.report.endTime = date_to_ms(2, 3, 2025) + std::chrono::hours(8);
		report.report.times.emplace_back(TaskID(3), 0);
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(1), TimeCodeID(2) }, std::chrono::hours(3));
		report.report.timePerTimeEntry.emplace(TimeEntry{ TimeCategoryID(2), TimeCodeID(3) }, std::chrono::hours(3));
		report.report.totalTime = std::chrono::hours(3);

		helper.required_messages({ &report });
	}
}

TEST_CASE("Request Weekly Report", "[api][task]")
{
	TestHelper helper;

	SECTION("No Report Found")
	{
		auto request = RequestWeeklyReportMessage(helper.next_request_id(), 6, 29, 2025);

		helper.api.process_packet(request, helper.output);

		auto report = WeeklyReportMessage(helper.prev_request_id());
		report.dailyReports[0].day = 29;
		report.dailyReports[1].day = 30;
		report.dailyReports[2].day = 1;
		report.dailyReports[3].day = 2;
		report.dailyReports[4].day = 3;
		report.dailyReports[5].day = 4;
		report.dailyReports[6].day = 5;

		report.dailyReports[0].month = 6;
		report.dailyReports[1].month = 6;
		report.dailyReports[2].month = 7;
		report.dailyReports[3].month = 7;
		report.dailyReports[4].month = 7;
		report.dailyReports[5].month = 7;
		report.dailyReports[6].month = 7;

		for (int i = 0; i < 7; i++)
		{
			report.dailyReports[i].year = 2025;
		}

		helper.required_messages({ &report });
	}
}

// TODO for now we're just going to do this in one big test
TEST_CASE("Persist Tasks", "[api][task]")
{
	TestClock clock;
	std::istringstream fileInput;
	std::ostringstream fileOutput;
	curlTest curl;
	API api(clock, curl, fileInput, fileOutput);
	std::vector<std::unique_ptr<Message>> output;

	auto create_task_1 = CreateTaskMessage(NO_PARENT, RequestID(1), "task 1");
	auto create_task_2 = CreateTaskMessage(TaskID(1), RequestID(2), "task 2");
	auto create_task_3 = CreateTaskMessage(TaskID(2), RequestID(3), "task 3");
	auto create_task_4 = CreateTaskMessage(TaskID(2), RequestID(4), "task 4");
	auto create_task_5 = CreateTaskMessage(TaskID(3), RequestID(5), "task 5");
	auto create_task_6 = CreateTaskMessage(TaskID(4), RequestID(6), "task 6");

	auto start_task_1 = TaskMessage(PacketType::START_TASK, RequestID(7), TaskID(1));
	auto start_task_2 = TaskMessage(PacketType::START_TASK, RequestID(8), TaskID(2));
	auto start_task_3 = TaskMessage(PacketType::START_TASK, RequestID(9), TaskID(3));
	auto start_task_4 = TaskMessage(PacketType::START_TASK, RequestID(10), TaskID(4));
	auto start_task_5 = TaskMessage(PacketType::START_TASK, RequestID(11), TaskID(5));
	auto start_task_6 = TaskMessage(PacketType::START_TASK, RequestID(12), TaskID(6));

	auto stop_task_2 = TaskMessage(PacketType::STOP_TASK, RequestID(13), TaskID(2));
	auto stop_task_3 = TaskMessage(PacketType::STOP_TASK, RequestID(14), TaskID(3));
	auto stop_task_4 = TaskMessage(PacketType::STOP_TASK, RequestID(15), TaskID(4));

	auto finish_task_2 = TaskMessage(PacketType::FINISH_TASK, RequestID(16), TaskID(2));
	auto finish_task_3 = TaskMessage(PacketType::FINISH_TASK, RequestID(17), TaskID(3));
	auto finish_task_4 = TaskMessage(PacketType::FINISH_TASK, RequestID(18), TaskID(4));

	auto rename = UpdateTaskMessage(RequestID(19), TaskID(1), NO_PARENT, "task 1 - rename");
	auto reparent = UpdateTaskMessage(RequestID(20), TaskID(2), NO_PARENT, "task 2");

	api.process_packet(create_task_1, output);
	api.process_packet(create_task_2, output);
	api.process_packet(start_task_2, output);
	api.process_packet(create_task_3, output);
	api.process_packet(stop_task_2, output);
	api.process_packet(start_task_3, output);
	api.process_packet(create_task_4, output);
	api.process_packet(create_task_5, output);
	api.process_packet(stop_task_3, output);
	api.process_packet(create_task_6, output);
	api.process_packet(start_task_2, output);
	api.process_packet(stop_task_2, output);
	api.process_packet(start_task_4, output);
	api.process_packet(finish_task_4, output);
	api.process_packet(finish_task_3, output);
	api.process_packet(finish_task_2, output);
	api.process_packet(start_task_6, output);
	api.process_packet(start_task_5, output);
	api.process_packet(start_task_1, output);
	api.process_packet(rename, output);
	api.process_packet(reparent, output);

	std::ostringstream expected;
	expected << "create 1 0 1737344939870 0 (6 task 1)\n";
	expected << "task-time-codes 1 0 0 \n";
	expected << "create 2 1 1737346739870 0 (6 task 2)\n";
	expected << "task-time-codes 2 0 0 \n";
	expected << "start 2 1737348539870 0 \n";
	expected << "create 3 2 1737350339870 0 (6 task 3)\n";
	expected << "task-time-codes 3 0 0 \n";
	expected << "stop 2 1737352139870\n";
	expected << "start 3 1737353939870 0 \n";
	expected << "create 4 2 1737355739870 0 (6 task 4)\n";
	expected << "task-time-codes 4 0 0 \n";
	expected << "create 5 3 1737357539870 0 (6 task 5)\n";
	expected << "task-time-codes 5 0 0 \n";
	expected << "stop 3 1737359339870\n";
	expected << "create 6 4 1737361139870 0 (6 task 6)\n";
	expected << "task-time-codes 6 0 0 \n";
	expected << "start 2 1737362939870 0 \n";
	expected << "stop 2 1737364739870\n";
	expected << "start 4 1737366539870 0 \n";
	expected << "finish 4 1737368339870\n";
	expected << "finish 3 1737370139870\n";
	expected << "finish 2 1737371939870\n";
	expected << "start 6 1737373739870 0 \n";
	expected << "start 5 1737376439870 0 \n";
	expected << "start 1 1737379139870 0 \n";
	expected << "rename 1 (15 task 1 - rename)\n";
	expected << "reparent 2 0\n";

	CHECK(fileOutput.str() == expected.str());
}

TEST_CASE("Reload Tasks From File", "[api]")
{
	std::istringstream fileInput;
	std::ostringstream fileOutput;

	fileOutput << "create 1 0 1737344939870 0 (6 task 1)\n";
	fileOutput << "task-time-codes 1 1 2 2 3 \n";
	fileOutput << "create 2 1 1737346739870 0 (6 task 2)\n";
	fileOutput << "start 2 1737348539870 0\n";
	fileOutput << "create 3 2 1737350339870 0 (6 task 3)\n";
	fileOutput << "stop 2 1737352139870\n";
	fileOutput << "start 3 1737353939870 0\n";
	fileOutput << "create 4 2 1737355739870 0 (6 task 4)\n";
	fileOutput << "create 5 3 1737357539870 0 (6 task 5)\n";
	fileOutput << "stop 3 1737359339870\n";
	fileOutput << "create 6 4 1737361139870 0 (6 task 6)\n";
	fileOutput << "start 2 1737362939870 0\n";
	fileOutput << "stop 2 1737364739870\n";
	fileOutput << "start 4 1737366539870 0\n";
	fileOutput << "finish 4 1737368339870\n";
	fileOutput << "finish 3 1737370139870\n";
	fileOutput << "finish 2 1737371939870\n";
	fileOutput << "start 6 1737373739870 0\n";
	fileOutput << "start 5 1737376439870 0\n";
	fileOutput << "start 1 1737379139870 1 2 2 3\n";
	fileOutput << "rename 1 (16 task 1 - renamed)\n";
	fileOutput << "reparent 2 0\n";

	fileInput = std::istringstream(fileOutput.str());
	fileOutput.clear();

	TestClock clock;
	curlTest curl;
	API api(clock, curl, fileInput, fileOutput);

	std::vector<std::unique_ptr<Message>> output;

	// now that we're setup, request the configuration and check the output
	api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, output);

	REQUIRE(output.size() == 8);

	auto task1 = TaskInfoMessage(TaskID(1), NO_PARENT, "task 1 - renamed");
	auto task2 = TaskInfoMessage(TaskID(2), NO_PARENT, "task 2");
	auto task3 = TaskInfoMessage(TaskID(3), TaskID(2), "task 3");
	auto task4 = TaskInfoMessage(TaskID(4), TaskID(2), "task 4");
	auto task5 = TaskInfoMessage(TaskID(5), TaskID(3), "task 5");
	auto task6 = TaskInfoMessage(TaskID(6), TaskID(4), "task 6");

	task1.state = TaskState::ACTIVE;
	task2.state = TaskState::FINISHED;
	task3.state = TaskState::FINISHED;
	task4.state = TaskState::FINISHED;
	task5.state = TaskState::INACTIVE;
	task6.state = TaskState::INACTIVE;

	task1.createTime = std::chrono::milliseconds(1737344939870);
	task2.createTime = std::chrono::milliseconds(1737346739870);
	task3.createTime = std::chrono::milliseconds(1737350339870);
	task4.createTime = std::chrono::milliseconds(1737355739870);
	task5.createTime = std::chrono::milliseconds(1737357539870);
	task6.createTime = std::chrono::milliseconds(1737361139870);

	task1.times.emplace_back(std::chrono::milliseconds(1737379139870), std::nullopt, std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} });
	task2.times.emplace_back(std::chrono::milliseconds(1737348539870), std::chrono::milliseconds(1737352139870));
	task2.times.emplace_back(std::chrono::milliseconds(1737362939870), std::chrono::milliseconds(1737364739870));
	task3.times.emplace_back(std::chrono::milliseconds(1737353939870), std::chrono::milliseconds(1737359339870));
	task4.times.emplace_back(std::chrono::milliseconds(1737366539870), std::chrono::milliseconds(1737368339870));
	task5.times.emplace_back(std::chrono::milliseconds(1737376439870), std::chrono::milliseconds(1737379139870));
	task6.times.emplace_back(std::chrono::milliseconds(1737373739870), std::chrono::milliseconds(1737376439870));

	task2.finishTime = std::chrono::milliseconds(1737371939870);
	task3.finishTime = std::chrono::milliseconds(1737370139870);
	task4.finishTime = std::chrono::milliseconds(1737368339870);

	task1.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

	verify_message(task1, *output[1]);
	verify_message(task2, *output[2]);
	verify_message(task3, *output[3]);
	verify_message(task4, *output[4]);
	verify_message(task5, *output[5]);
	verify_message(task6, *output[6]);
	verify_message(BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *output[7]);
}

TEST_CASE("Persist Time Categories", "[api]")
{
	TestClock clock;
	std::istringstream fileInput;
	std::ostringstream fileOutput;
	curlTest curl;
	API api(clock, curl, fileInput, fileOutput);
	std::vector<std::unique_ptr<Message>> output;

	auto create_task_1 = CreateTaskMessage(NO_PARENT, RequestID(1), "task 1");

	api.process_packet(create_task_1, output);

	std::vector<TimeCategory> timeCategories;
	TimeCategory tg1{ TimeCategoryID(0), "Te st", "TS T" };
	tg1.codes.push_back(TimeCode{ TimeCodeID(0), "Fo o" });
	tg1.codes.push_back(TimeCode{ TimeCodeID(0), "Ba r" });
	tg1.codes.push_back(TimeCode{ TimeCodeID(0), "Buz z" });
	timeCategories.push_back(tg1);

	auto add_category1 = TimeEntryModifyPacket(RequestID(2), TimeCategoryModType::ADD, timeCategories);

	api.process_packet(add_category1, output);

	timeCategories.clear();
	TimeCategory tg2{ TimeCategoryID(0), "Tw o", "TW O" };
	tg2.codes.push_back(TimeCode{ TimeCodeID(0), "Biz z" });
	timeCategories.push_back(tg2);

	auto add_category2 = TimeEntryModifyPacket(RequestID(3), TimeCategoryModType::ADD, timeCategories);

	api.process_packet(add_category2, output);

	timeCategories.clear();

	tg1.id = TimeCategoryID(1);
	tg1.name = "Test er";
	tg1.label = "TST R";
	tg1.codes.clear();
	tg1.codes.push_back(TimeCode{ TimeCodeID(1), "Fo o" });
	tg1.codes.push_back(TimeCode{ TimeCodeID(2), "Bar s" });
	tg1.codes.push_back(TimeCode{ TimeCodeID(3), "Buz z" });
	timeCategories.push_back(tg1);

	auto update_category = TimeEntryModifyPacket(RequestID(4), TimeCategoryModType::UPDATE, timeCategories);
	api.process_packet(update_category, output);

	timeCategories.clear();
	tg1.id = TimeCategoryID(1);
	tg1.name = "Test er";
	tg1.label = "TST R";
	tg1.codes.clear();
	tg1.codes.push_back(TimeCode{ TimeCodeID(2), "Bar s" });
	tg1.codes.push_back(TimeCode{ TimeCodeID(3), "Buz z" });
	timeCategories.push_back(tg1);

	auto remove_code = TimeEntryModifyPacket(RequestID(5), TimeCategoryModType::REMOVE_CODE, timeCategories);
	api.process_packet(remove_code, output);
	
	timeCategories.clear();
	tg1.id = TimeCategoryID(1);
	tg1.name = "Tester";
	tg1.label = "TSTR";
	timeCategories.push_back(tg1);

	auto remove_category = TimeEntryModifyPacket(RequestID(5), TimeCategoryModType::REMOVE_CATEGORY, timeCategories);
	api.process_packet(remove_category, output);

	std::ostringstream expected;
	expected << "create 1 0 1737344939870 0 (6 task 1)\n";
	expected << "task-time-codes 1 0 0 \n";
	expected << "time-category add 0 (5 Te st) (4 TS T) 3 0 (4 Fo o) 0 (4 Ba r) 0 (5 Buz z) \n";
	expected << "time-category add 0 (4 Tw o) (4 TW O) 1 0 (5 Biz z) \n";
	expected << "time-category update 1 (7 Test er) (5 TST R) 3 1 (4 Fo o) 2 (5 Bar s) 3 (5 Buz z) \n";
	expected << "time-category remove-code 1 { 2 3 }\n";
	expected << "time-category remove-category 1\n";

	CHECK(fileOutput.str() == expected.str());

	output.clear();

	// now that we're setup, request the configuration and check the output
	api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, output);

	REQUIRE(output.size() == 3);

	auto timeCategoriesData = TimeEntryDataPacket({});
	auto& cat2 = timeCategoriesData.timeCategories.emplace_back(TimeCategoryID(2), "Tw o", "TW O");
	cat2.codes.emplace_back(TimeCodeID(4), "Biz z");

	verify_message(timeCategoriesData, *output[0]);
}

TEST_CASE("Reload Time Categories", "[api]")
{
	std::istringstream fileInput;
	std::ostringstream fileOutput;

	fileOutput << "create 1 0 1737344939870 0 (6 task 1)\n";
	fileOutput << "time-category add 0 (5 Te st) (4 TS T) 2 0 (4 Fo o) 0 (4 Ba r)\n";
	fileOutput << "time-category add 1 (5 Te st) (4 TS T) 3 1 (4 Fo o) 2 (4 Ba r) 0 (5 Buz z)\n";
	fileOutput << "time-category add 0 (4 Tw o) (4 TW O) 1 0 (5 Biz z) \n";
	fileOutput << "time-category update 1 (7 Test er) (5 TST R) 1 2 (5 Bar s)\n";
	fileOutput << "time-category remove-code 1 { 2 3 }\n";
	fileOutput << "time-category remove-category 1\n";

	fileInput = std::istringstream(fileOutput.str());
	fileOutput.clear();

	TestClock clock;
	curlTest curl;
	API api(clock, curl, fileInput, fileOutput);

	std::vector<std::unique_ptr<Message>> output;

	// now that we're setup, request the configuration and check the output
	api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, output);

	REQUIRE(output.size() == 3);

	auto timeCategoriesData = TimeEntryDataPacket({});
	auto& cat2 = timeCategoriesData.timeCategories.emplace_back(TimeCategoryID(2), "Tw o", "TW O");
	cat2.codes.emplace_back(TimeCodeID(4), "Biz z");

	verify_message(timeCategoriesData, *output[0]);

	auto task1 = TaskInfoMessage(TaskID(1), NO_PARENT, "task 1");
	task1.state = TaskState::INACTIVE;
	task1.createTime = std::chrono::milliseconds(1737344939870);

	verify_message(task1, *output[1]);

	verify_message(BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *output[2]);
}

TEST_CASE("request configuration at startup", "[api]")
{
	TestClock clock;
	std::istringstream fileInput;
	std::ostringstream fileOutput;
	curlTest curl;
	API api(clock, curl, fileInput, fileOutput);

	std::vector<std::unique_ptr<Message>> output;

	auto create_task_1 = CreateTaskMessage(NO_PARENT, RequestID(1), "task 1");
	auto create_task_2 = CreateTaskMessage(TaskID(1), RequestID(2), "task 2");
	auto create_task_3 = CreateTaskMessage(TaskID(2), RequestID(3), "task 3");
	auto create_task_4 = CreateTaskMessage(TaskID(2), RequestID(4), "task 4");
	auto create_task_5 = CreateTaskMessage(TaskID(3), RequestID(5), "task 5");
	auto create_task_6 = CreateTaskMessage(TaskID(4), RequestID(6), "task 6");

	api.process_packet(create_task_1, output);
	api.process_packet(create_task_2, output);
	api.process_packet(create_task_3, output);
	api.process_packet(create_task_4, output);
	api.process_packet(create_task_5, output);
	api.process_packet(create_task_6, output);

	auto timeCategories = TimeEntryModifyPacket(RequestID(1), TimeCategoryModType::ADD, {});
	auto& category1 = timeCategories.timeCategories.emplace_back(TimeCategoryID(0), "Foo", "F");
	category1.codes.emplace_back(TimeCodeID(0), "Fizz");
	category1.codes.emplace_back(TimeCodeID(0), "Buzz");

	auto& category2 = timeCategories.timeCategories.emplace_back(TimeCategoryID(0), "Bar", "B");
	category2.codes.emplace_back(TimeCodeID(0), "Bing");
	category2.codes.emplace_back(TimeCodeID(0), "Bong");

	output.clear();

	// now that we're setup, request the configuration and check the output
	api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, output);

	REQUIRE(output.size() == 8);

	auto timeCategoriesData = TimeEntryDataPacket({});

	verify_message(timeCategoriesData, *output[0]);

	verify_message(TaskInfoMessage(TaskID(1), NO_PARENT, "task 1", std::chrono::milliseconds(1737344039870)), *output[1]);
	verify_message(TaskInfoMessage(TaskID(2), TaskID(1), "task 2", std::chrono::milliseconds(1737345839870)), *output[2]);
	verify_message(TaskInfoMessage(TaskID(3), TaskID(2), "task 3", std::chrono::milliseconds(1737347639870)), *output[3]);
	verify_message(TaskInfoMessage(TaskID(4), TaskID(2), "task 4", std::chrono::milliseconds(1737349439870)), *output[4]);
	verify_message(TaskInfoMessage(TaskID(5), TaskID(3), "task 5", std::chrono::milliseconds(1737351239870)), *output[5]);
	verify_message(TaskInfoMessage(TaskID(6), TaskID(4), "task 6", std::chrono::milliseconds(1737353039870)), *output[6]);

	verify_message(BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *output[7]);
}
