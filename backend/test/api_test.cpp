#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "api.hpp"
#include "server.hpp"
#include "packets.hpp"
#include "utils.h"

#include <vector>
#include <source_location>

using namespace std::chrono_literals;

TEST_CASE("no parent ID is 0", "[task]")
{
	CHECK(NO_PARENT == TaskID(0));
}

TEST_CASE("Create Task", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	SECTION("Success")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "this is a test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = true;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Create Task With Parent")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(TaskID(1), helper.next_request_id(), "test 2"));

		auto taskInfo = TaskInfoMessage(TaskID(2), TaskID(1), "test 2");

		taskInfo.createTime = std::chrono::milliseconds(1737344939870);
		taskInfo.state = TaskState::PENDING;
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
		taskInfo.state = TaskState::PENDING;
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
}

TEST_CASE("Start Task", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	SECTION("Success")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737345839870), std::nullopt));
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
		taskInfo1.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737345839870), std::chrono::milliseconds(1737346739870)));
		taskInfo1.state = TaskState::PENDING;
		taskInfo1.newTask = false;

		auto taskInfo2 = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");

		taskInfo2.createTime = std::chrono::milliseconds(1737344939870);
		taskInfo2.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737346739870), std::nullopt));
		taskInfo2.state = TaskState::ACTIVE;
		taskInfo2.newTask = false;
		taskInfo2.indexInParent = 1;

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
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737344939870), std::nullopt));
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
		taskInfo.finishTime = std::chrono::milliseconds(1737344939870);
		taskInfo.state = TaskState::FINISHED;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}
}

TEST_CASE("Start Task - Time Entry", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
	auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

	auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B");
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
		times.start = std::chrono::milliseconds(1737344939870);
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
		taskInfo.createTime = std::chrono::milliseconds(1737344939870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737345839870);
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
		taskInfo.createTime = std::chrono::milliseconds(1737344939870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737345839870);
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
		taskInfo.createTime = std::chrono::milliseconds(1737345839870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737346739870);
		times.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		taskInfo.times.push_back(times);
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Inherit Time Code from Time Category That Task Does Not Have a Time Code For")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		
		auto& newCategory3 = modify.timeCategories.emplace_back(TimeCategoryID(0), "C");
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
		taskInfo.createTime = std::chrono::milliseconds(1737345839870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737346739870);
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
		taskInfo.createTime = std::chrono::milliseconds(1737345839870);

		TaskTimes times;
		times.start = std::chrono::milliseconds(1737346739870);
		times.timeEntry = std::vector{ TimeEntry{ TimeCategoryID(1), TimeCodeID(0) }, TimeEntry{ TimeCategoryID(2), TimeCodeID(0) } };

		taskInfo.times.push_back(times);
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}
}

TEST_CASE("Start Task - Empty Time Entry", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	CreateTaskMessage create(NO_PARENT, helper.next_request_id(), "test 1");
	
	helper.expect_success(create);

	helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

	auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

	taskInfo.createTime = std::chrono::milliseconds(1737344039870);

	TaskTimes times;
	times.start = std::chrono::milliseconds(1737344939870);
	times.timeEntry = std::vector{ TimeEntry{ TimeCategoryID(0), TimeCodeID(0) } };

	taskInfo.times.push_back(times);
	taskInfo.state = TaskState::ACTIVE;
	taskInfo.newTask = false;

	helper.required_messages({ &taskInfo });
}

TEST_CASE("Stop Task", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	SECTION("Success - Stop Active Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737344939870), std::chrono::milliseconds(1737345839870)));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Stopping Active Task Clears Active Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1))); // stop the task. this step is critical to the test
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1))); // finish the task to force it out of PENDING state

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));

		auto taskInfo = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");

		taskInfo.createTime = std::chrono::milliseconds(1737344939870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737348539870), std::nullopt));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;
		taskInfo.indexInParent = 1;

		helper.required_messages({ &taskInfo });

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737345839870), std::chrono::milliseconds(1737346739870)));
		taskInfo.finishTime = std::chrono::milliseconds(1737347639870);
		taskInfo.state = TaskState::FINISHED;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Task Does Not Exist")
	{
		helper.expect_failure(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Active Task is Not Stopped When Task Does Not Exist")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_failure(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(2)), "Task with ID 2 does not exist.");

		// we can stop the task because it should still be active
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
	}

	SECTION("Failure - Unspecified Task is Not Stopped When Task Does Not Exist")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

		helper.expect_failure(TaskMessage(PacketType::STOP_UNSPECIFIED_TASK, helper.next_request_id(), TaskID(2)), "Task with ID 2 does not exist.");

		// we can stop the unspecified task now because it should still be active
		helper.expect_success(TaskMessage(PacketType::STOP_UNSPECIFIED_TASK, helper.next_request_id(), TaskID(1)));
	}

	SECTION("Failure - Task Is Not Active")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));

		helper.expect_failure(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)), "Task with ID 1 is not active.");

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::PENDING;
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
		taskInfo.finishTime = std::chrono::milliseconds(1737344939870);
		taskInfo.state = TaskState::FINISHED;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}
}

TEST_CASE("Finish Task", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	SECTION("Success - Finish Active Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737344939870), std::chrono::milliseconds(1737345839870)));
		taskInfo.finishTime = std::chrono::milliseconds(1737345839870);
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

			taskInfo.createTime = std::chrono::milliseconds(1737344939870);
			taskInfo.finishTime = std::chrono::milliseconds(1737346739870);
			taskInfo.state = TaskState::FINISHED;
			taskInfo.newTask = false;
			taskInfo.indexInParent = 1;

			helper.required_messages({ &taskInfo });

			// active task remains active and hasn't been modified
			helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

			taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

			taskInfo.createTime = std::chrono::milliseconds(1737344039870);
			taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737345839870), std::nullopt));
			taskInfo.state = TaskState::ACTIVE;
			taskInfo.newTask = false;

			helper.required_messages({ &taskInfo });
		}

		SECTION("Without Active Task")
		{
			helper.expect_success(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), TaskID(2)));

			auto taskInfo = TaskInfoMessage(TaskID(2), NO_PARENT, "test 2");

			taskInfo.createTime = std::chrono::milliseconds(1737344939870);
			taskInfo.finishTime = std::chrono::milliseconds(1737345839870);
			taskInfo.state = TaskState::FINISHED;
			taskInfo.newTask = false;
			taskInfo.indexInParent = 1;

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

		taskInfo.createTime = std::chrono::milliseconds(1737344939870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737347639870), std::nullopt));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;
		taskInfo.indexInParent = 1;

		helper.required_messages({ &taskInfo });

		// task hasn't been modified
		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737345839870), std::chrono::milliseconds(1737346739870)));
		taskInfo.finishTime = std::chrono::milliseconds(1737346739870);
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
		taskInfo1.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737346739870), std::chrono::milliseconds(1737348539870)));
		taskInfo1.state = TaskState::PENDING;
		taskInfo1.newTask = false;

		auto taskInfo3 = TaskInfoMessage(TaskID(3), NO_PARENT, "test 3");

		taskInfo3.createTime = std::chrono::milliseconds(1737345839870);
		taskInfo3.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737348539870), std::nullopt));
		taskInfo3.state = TaskState::ACTIVE;
		taskInfo3.newTask = false;
		taskInfo3.indexInParent = 2;

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
}

TEST_CASE("Modify Task", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	SECTION("Success - Rename Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(UpdateTaskMessage(helper.next_request_id(), TaskID(1), NO_PARENT, "something else"));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "something else");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::PENDING;
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
		taskInfo.state = TaskState::PENDING;
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

	SECTION("Success - Change Task Time Codes")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));

		UpdateTaskMessage update(helper.next_request_id(), TaskID(1), NO_PARENT, "test");
		update.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		// TODO validate that time codes are valid
		helper.expect_success(update);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;
		taskInfo.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.required_messages({ &taskInfo });
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
	// - task cannot be changed back to pending from finished if using an archived time category or time code (I don't think this feature exists yet)
	TestHelper<nullDatabase> helper;

	SECTION("Success - Add Time Category")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		auto& newCategory = modify.timeCategories.emplace_back(TimeCategoryID(0), "New");
		newCategory.codes.emplace_back(TimeCodeID(0), "Code 1");

		helper.expect_success(modify);

		auto data = TimeEntryDataPacket({});
		auto& verifyCategory = data.timeCategories.emplace_back(TimeCategoryID(1), "New");
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
	TestHelper<nullDatabase> helper;

	auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
	auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

	auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 5");

	helper.expect_success(modify);

	helper.clear_message_output();

	SECTION("No Report Found")
	{
		auto request = RequestDailyReportMessage(helper.next_request_id(), 2, 3, 2025);

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1737344039870));
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738560600000));
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5) + std::chrono::minutes(15);
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738647000000));
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5) + std::chrono::minutes(15);
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738542600000));
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738560600000));
		report.report.found = true;
		report.report.month = 2;
		report.report.day = 3;
		report.report.year = 2025;
		report.report.startTime = date_to_ms(2, 3, 2025) + std::chrono::hours(5) + std::chrono::minutes(15);
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738566000000));
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738566000000));
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738652400000));
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
		auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "C");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 6");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 7");

		auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "D");
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738659600000));
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

		helper.api.process_packet(request);

		auto report = DailyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1738569600000));
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
	TestHelper<nullDatabase> helper;

	SECTION("No Report Found")
	{
		auto request = RequestWeeklyReportMessage(helper.next_request_id(), 6, 29, 2025);

		helper.api.process_packet(request);

		auto report = WeeklyReportMessage(helper.prev_request_id(), std::chrono::milliseconds(1737344039870));
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

TEST_CASE("request configuration at startup", "[api]")
{
	TestClock clock;
	curlTest curl;
	nullDatabase db;
	TestPacketSender sender;
	API api(clock, curl, db, sender);

	auto create_task_1 = CreateTaskMessage(NO_PARENT, RequestID(1), "task 1");
	auto create_task_2 = CreateTaskMessage(TaskID(1), RequestID(2), "task 2");
	auto create_task_3 = CreateTaskMessage(TaskID(2), RequestID(3), "task 3");
	auto create_task_4 = CreateTaskMessage(TaskID(2), RequestID(4), "task 4");
	auto create_task_5 = CreateTaskMessage(TaskID(3), RequestID(5), "task 5");
	auto create_task_6 = CreateTaskMessage(TaskID(4), RequestID(6), "task 6");

	api.process_packet(create_task_1);
	api.process_packet(create_task_2);
	api.process_packet(create_task_3);
	api.process_packet(create_task_4);
	api.process_packet(create_task_5);
	api.process_packet(create_task_6);

	auto timeCategories = TimeEntryModifyPacket(RequestID(1), TimeCategoryModType::ADD, {});
	auto& category1 = timeCategories.timeCategories.emplace_back(TimeCategoryID(0), "Foo");
	category1.codes.emplace_back(TimeCodeID(0), "Fizz");
	category1.codes.emplace_back(TimeCodeID(0), "Buzz");

	auto& category2 = timeCategories.timeCategories.emplace_back(TimeCategoryID(0), "Bar");
	category2.codes.emplace_back(TimeCodeID(0), "Bing");
	category2.codes.emplace_back(TimeCodeID(0), "Bong");

	sender.output.clear();

	// now that we're setup, request the configuration and check the output
	api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION });

	REQUIRE(sender.output.size() == 10);

	auto timeCategoriesData = TimeEntryDataPacket({});

	verify_message(timeCategoriesData, *sender.output[0]);

	verify_message(TaskInfoMessage(TaskID(1), NO_PARENT, "task 1", std::chrono::milliseconds(1737344039870)), *sender.output[2]);
	verify_message(TaskInfoMessage(TaskID(2), TaskID(1), "task 2", std::chrono::milliseconds(1737344939870)), *sender.output[3]);
	verify_message(TaskInfoMessage(TaskID(3), TaskID(2), "task 3", std::chrono::milliseconds(1737345839870)), *sender.output[4]);
	auto task4 = TaskInfoMessage(TaskID(4), TaskID(2), "task 4", std::chrono::milliseconds(1737346739870));
	task4.indexInParent = 1;
	verify_message(task4, *sender.output[5]);
	verify_message(TaskInfoMessage(TaskID(5), TaskID(3), "task 5", std::chrono::milliseconds(1737347639870)), *sender.output[6]);
	verify_message(TaskInfoMessage(TaskID(6), TaskID(4), "task 6", std::chrono::milliseconds(1737348539870)), *sender.output[7]);

	verify_message(BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *sender.output[9]);
}

TEST_CASE("Request Version", "[api]")
{
	TestClock clock;
	curlTest curl;
	nullDatabase db;
	TestPacketSender sender;
	API api(clock, curl, db, sender);

	auto request = BasicMessage(PacketType::VERSION_REQUEST);

	api.process_packet(request);

	REQUIRE(sender.output.size() == 1);

	verify_message(VersionMessage("0.13.1"), *sender.output[0]);
}

TEST_CASE("Start Unspecified Task", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));

	SECTION("Success - Start Unspecified Task when There is No Active Task")
	{
		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

		auto active = BasicMessage(PacketType::UNSPECIFIED_TASK_ACTIVE);

		helper.required_messages({ &active });
	}

	SECTION("Success - Start Unspecified Task when There is an Active Task (other than unspecified)")
	{
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

		auto taskInfo1 = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

		taskInfo1.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo1.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737344939870), std::chrono::milliseconds(1737345839870)));
		taskInfo1.state = TaskState::PENDING;
		taskInfo1.newTask = false;

		auto active = BasicMessage(PacketType::UNSPECIFIED_TASK_ACTIVE);

		helper.required_messages({ &taskInfo1, &active });
	}

	SECTION("Failure - Does Not Start when Unspecified Task is Active Task")
	{
		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

		helper.expect_failure(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK), "Unspecified task is already active.");
	}

	SECTION("Failure - Cannot Start Normal Task While Unspecified is Active")
	{
		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

		helper.expect_failure(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)), "Cannot start task with ID 1. Unspecified task is active.");
	}

	SECTION("Failure - Cannot Stop Unspecified Task Like Normal Task")
	{
		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

		helper.expect_failure(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), UNSPECIFIED_TASK), "Unspecified task cannot be stopped.");
	}

	SECTION("Failure - Cannot Finish Unspecified Task Like Normal Task")
	{
		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

		helper.expect_failure(TaskMessage(PacketType::FINISH_TASK, helper.next_request_id(), UNSPECIFIED_TASK), "Unspecified task cannot be finished.");
	}
}

TEST_CASE("Stop Unspecified Task", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));

	helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

	helper.expect_success(TaskMessage(PacketType::STOP_UNSPECIFIED_TASK, helper.next_request_id(), TaskID(1)));

	auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

	taskInfo.createTime = std::chrono::milliseconds(1737344039870);
	taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737344939870), std::nullopt));
	taskInfo.state = TaskState::ACTIVE;
	taskInfo.newTask = false;

	helper.required_messages({ &taskInfo });

	// verify that we can start the unspecified task again
	helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));
}

TEST_CASE("Starting Unspecified Task Multiple Times Clears Times", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));

	helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

	helper.expect_success(TaskMessage(PacketType::STOP_UNSPECIFIED_TASK, helper.next_request_id(), TaskID(1)));

	helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));

	helper.expect_success(TaskMessage(PacketType::STOP_UNSPECIFIED_TASK, helper.next_request_id(), TaskID(1)));

	auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "test 1");

	taskInfo.createTime = std::chrono::milliseconds(1737344039870);
	taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737344939870), std::chrono::milliseconds(1737346739870)));
	taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737346739870), std::nullopt));
	taskInfo.state = TaskState::ACTIVE;
	taskInfo.newTask = false;

	helper.required_messages({ &taskInfo });
}

TEST_CASE("Prevent Reparenting Mistakes", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	SECTION("Reparent to Self")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));

		helper.expect_failure(UpdateTaskMessage(helper.next_request_id(), TaskID(1), TaskID(1), "a"), "Cannot reparent Task with ID 1 to itself");

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737344939870), std::nullopt));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Simple")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "b"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "c"));

		helper.expect_success(UpdateTaskMessage(helper.next_request_id(), TaskID(2), TaskID(1), "b"));

		helper.expect_failure(UpdateTaskMessage(helper.next_request_id(), TaskID(1), TaskID(2), "a"), "Cannot reparent Task with ID 1 to Task with ID 2. Task with ID 1 is a descendant of Task with ID 2.");

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737346739870), std::nullopt));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Deep Recursion")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(CreateTaskMessage(TaskID(1), helper.next_request_id(), "b"));
		helper.expect_success(CreateTaskMessage(TaskID(2), helper.next_request_id(), "c"));
		helper.expect_success(CreateTaskMessage(TaskID(3), helper.next_request_id(), "d"));

		helper.expect_success(UpdateTaskMessage(helper.next_request_id(), TaskID(2), TaskID(1), "b"));

		helper.expect_failure(UpdateTaskMessage(helper.next_request_id(), TaskID(1), TaskID(2), "a"), "Cannot reparent Task with ID 1 to Task with ID 2. Task with ID 1 is a descendant of Task with ID 2.");

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(std::chrono::milliseconds(1737347639870), std::nullopt));
		taskInfo.state = TaskState::ACTIVE;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}
}

TEST_CASE("Add Sessions", "[api][task]")
{
	// check that the new session times don't overlap with any session from any task
	TestHelper<nullDatabase> helper;

	SECTION("Success - Add Session to Empty Task")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

		auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B");
		newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
		newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");

		helper.expect_success(modify);

		auto create = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a");
		create.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create);

		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);

		helper.expect_success(add);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.timeEntry = create.timeEntry;
		taskInfo.times.push_back(create_times_with_unknown_time_entry(10000ms, 20000ms));
		taskInfo.times.back().timeEntry = create.timeEntry;
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Does Not Overlap with Task Sessions")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);

		helper.expect_success(add);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(10000ms, 20000ms));
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737344939870ms, 1737345839870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Does Not Overlap with Other Task Sessions")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(CreateTaskMessage(TaskID(1), helper.next_request_id(), "b"));
		helper.expect_success(CreateTaskMessage(TaskID(2), helper.next_request_id(), "c"));
		helper.expect_success(CreateTaskMessage(TaskID(3), helper.next_request_id(), "d"));

		// start/stop each of the tasks
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(4)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(4)));

		// add new times that won't overlap
		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);

		helper.expect_success(add);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(10000ms, 20000ms));
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737347639870ms, 1737348539870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - When Checking for Overlap, Change is Not Applied")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

		auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B");
		newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
		newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");

		helper.expect_success(modify);

		auto create = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a");
		create.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

		helper.expect_success(create);

		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);
		add.checkForOverlaps = true;

		helper.expect_success(add);

		helper.required_messages({});

		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.timeEntry = create.timeEntry;
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Use Unknown from Each Time Category When Task has no Time Entry")
	{
		auto modify = TimeEntryModifyPacket(helper.next_request_id(), TimeCategoryModType::ADD, {});
		auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
		newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

		auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B");
		newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
		newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");

		helper.expect_success(modify);

		auto create = CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a");
		helper.expect_success(create);

		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);

		helper.expect_success(add);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.emplace_back(10000ms, 20000ms, std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(0)}, TimeEntry{TimeCategoryID(2), TimeCodeID(0)} });
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Unknown Task ID")
	{
		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);

		helper.expect_failure(add, "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Stop Cannot be Before Start")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), 20000ms, 10000ms);

		helper.expect_failure(add, "Stop time cannot be before start time.");
	}

	SECTION("Failure - Overlaps with Task Sessions")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto start = 1737344939870ms;
		auto stop = 1737345839870ms;

		// overlap start time of first session (existing start time inside new session)
		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), start - 50ms, start + 50ms);

		helper.expect_failure(add, "Overlap detected with 'a'.");

		// overlap start time of first session (new start time inside existing session)
		add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), start + 50ms, stop + 50ms);

		helper.expect_failure(add, "Overlap detected with 'a'.");

		// overlap stop time of first session (existing stop time inside new session)
		add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), stop - 50ms, stop + 50ms);

		helper.expect_failure(add, "Overlap detected with 'a'.");

		// overlap stop time of first session (new stop time inside existing session)
		add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), start - 50ms, start + 50ms);

		helper.expect_failure(add, "Overlap detected with 'a'.");

		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737344939870ms, 1737345839870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Overlaps with Other Task Sessions")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(CreateTaskMessage(TaskID(1), helper.next_request_id(), "b"));
		helper.expect_success(CreateTaskMessage(TaskID(2), helper.next_request_id(), "c"));
		helper.expect_success(CreateTaskMessage(TaskID(3), helper.next_request_id(), "d"));

		// start/stop each of the tasks
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(4)));

		helper.clock.time += 100s;

		// task 2 times
		auto start = 1737348639870ms;
		auto stop = 1737349539870ms;

		// overlap start time of task id 2 session
		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), start - 50ms, start + 50ms);

		helper.expect_failure(add, "Overlap detected with 'b'.");

		// overlap stop time of task id 2 session
		add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), stop - 50ms, stop + 50ms);

		helper.expect_failure(add, "Overlap detected with 'c'.");

		auto active_start = 1737350439870ms;

		// overlap active time of task id 4 session (stop time only)
		add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), active_start - 50ms, active_start + 1000s);

		helper.expect_failure(add, "Overlap detected with 'd'.");

		// overlap active time of task id 4 session (start time and stop time)
		add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), active_start + 50ms, active_start + 1000s);

		helper.expect_failure(add, "Overlap detected with 'd'.");

		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737347639870ms, 1737348539870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - New Session Must Have Start and Stop Times")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(CreateTaskMessage(TaskID(1), helper.next_request_id(), "b"));
		helper.expect_success(CreateTaskMessage(TaskID(2), helper.next_request_id(), "c"));
		helper.expect_success(CreateTaskMessage(TaskID(3), helper.next_request_id(), "d"));

		// start/stop each of the tasks
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(3)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(4)));

		auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, std::nullopt);

		helper.expect_failure(add, "New session must have a stop time.");

		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737347639870ms, 1737348539870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	// TODO what to do with time entry? I think it just gets set to whatever is on the task, then the user can edit it later
}

TEST_CASE("Edit Sessions", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	// check that the modified session times don't overlap with any sessions in the task
	// check that the modified session times don't overlap with any session from any other task

	SECTION("Success - Edit Start Time")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 1737345839870ms);
		edit.sessionIndex = 0;

		helper.expect_success(edit);

		edit.sessionIndex = 1;
		edit.start = 20000ms;
		edit.stop = 1737347639870ms;

		helper.expect_success(edit);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(10000ms, 1737345839870ms));
		taskInfo.times.push_back(create_times_with_unknown_time_entry(20000ms, 1737347639870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Edit Stop Time")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), 5000ms, 10000ms);
		edit.sessionIndex = 0;

		helper.expect_success(edit);

		edit.sessionIndex = 1;
		edit.start = 11000ms;
		edit.stop = 20000ms;

		helper.expect_success(edit);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(5000ms, 10000ms));
		taskInfo.times.push_back(create_times_with_unknown_time_entry(11000ms, 20000ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - When Checking for Overlap, Change is Not Applied")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 1737345839870ms);
		edit.sessionIndex = 0;
		edit.checkForOverlaps = true;

		helper.expect_success(edit);

		helper.required_messages({});

		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737344939870ms, 1737345839870ms));
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737346739870ms, 1737347639870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Unknown Task ID")
	{
		auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);

		helper.expect_failure(edit, "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Invalid Session Index")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), 1737344939870ms, 1737345839870ms);
		edit.sessionIndex = 2;

		helper.expect_failure(edit, "Invalid session index.");
	}

	SECTION("Failure - Stop Cannot be Before Start")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), 20000ms, 10000ms);
		edit.sessionIndex = 0;

		helper.expect_failure(edit, "Stop time cannot be before start time.");
	}

	SECTION("Failure - Cannot Change Stop Present State")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));

		auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, std::nullopt);
		edit.sessionIndex = 0;

		helper.expect_failure(edit, "Cannot remove stop time.");

		edit.sessionIndex = 1;
		edit.stop = 20000ms;

		helper.expect_failure(edit, "Cannot add stop time.");
	}

	SECTION("Failure - Overlaps with Task Sessions")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "b"));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(2)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(2)));

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto start = 1737345839870ms;
		auto stop = 1737346739870ms;

		// overlap start time of first session (existing start time inside new session)
		auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), start - 50ms, start + 50ms);

		helper.expect_failure(edit, "Overlap detected with 'b'.");

		// overlap start time of first session (new start time inside existing session)
		edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), start + 50ms, stop + 50ms);

		helper.expect_failure(edit, "Overlap detected with 'b'.");

		// overlap stop time of first session (existing stop time inside new session)
		edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), stop - 50ms, stop + 50ms);

		helper.expect_failure(edit, "Overlap detected with 'b'.");

		// overlap stop time of first session (new stop time inside existing session)
		edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, helper.next_request_id(), TaskID(1), start - 50ms, start + 50ms);

		helper.expect_failure(edit, "Overlap detected with 'b'.");

		helper.expect_success(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(1)));

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.state = TaskState::PENDING;
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737347639870ms, 1737348539870ms));
		taskInfo.newTask = false;
		helper.required_messages({ &taskInfo });
	}
}

TEST_CASE("Remove Sessions", "[api][task]")
{
	TestHelper<nullDatabase> helper;

	
	// remove the first session
	SECTION("Success - Remove First Session")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto remove = UpdateTaskTimesMessage(PacketType::REMOVE_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);
		remove.sessionIndex = 0;

		helper.expect_success(remove);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737346739870ms, 1737347639870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Success - Remove Second Session")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		auto remove = UpdateTaskTimesMessage(PacketType::REMOVE_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);
		remove.sessionIndex = 1;

		helper.expect_success(remove);

		auto taskInfo = TaskInfoMessage(TaskID(1), NO_PARENT, "a");

		taskInfo.createTime = std::chrono::milliseconds(1737344039870);
		taskInfo.times.push_back(create_times_with_unknown_time_entry(1737344939870ms, 1737345839870ms));
		taskInfo.state = TaskState::PENDING;
		taskInfo.newTask = false;

		helper.required_messages({ &taskInfo });
	}

	SECTION("Failure - Unknown Task ID")
	{
		auto remove = UpdateTaskTimesMessage(PacketType::REMOVE_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);

		helper.expect_failure(remove, "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Invalid Session Index")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "a"));

		auto remove = UpdateTaskTimesMessage(PacketType::REMOVE_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);
		remove.sessionIndex = 0;

		helper.expect_failure(remove, "Invalid session index.");

		helper.expect_success(TaskMessage(PacketType::START_TASK, helper.next_request_id(), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::STOP_TASK, helper.next_request_id(), TaskID(1)));

		remove = UpdateTaskTimesMessage(PacketType::REMOVE_TASK_SESSION, helper.next_request_id(), TaskID(1), 10000ms, 20000ms);
		remove.sessionIndex = 1;

		helper.expect_failure(remove, "Invalid session index.");
	}
}
