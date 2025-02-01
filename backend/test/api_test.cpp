#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "server.hpp"
#include "packets.hpp"

#include <vector>
#include <source_location>

template<typename T>
void verify_message(const T& expected, const Message& actual, std::source_location location = std::source_location::current())
{
	INFO(location.file_name() << ":" << location.line());

	UNSCOPED_INFO("packet type: " << static_cast<std::int32_t>(actual.packetType()));
	
	if (const auto* actual_message = dynamic_cast<const T*>(&actual))
	{
		CHECK(*actual_message == expected);
	}
	else
	{
		UNSCOPED_INFO("expected message: " << expected);
		FAIL();
	}
}

struct TestHelper
{
	TestClock clock;

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, fileInput, fileOutput);

	std::vector<std::unique_ptr<Message>> output;

	RequestID next_request_id()
	{
		auto id = m_next_request_id++;
		m_prev_request_id = id;
		return id;
	}

	RequestID prev_request_id() const
	{
		return m_prev_request_id;
	}

	void clear_output()
	{
		fileOutput.str("");
	}

	void expect_success(const RequestMessage& message, std::source_location location = std::source_location::current())
	{
		output.clear();

		api.process_packet(message, output);

		// check output for a success message for the request ID
		if (!output.empty())
		{
			INFO("First message sent should be the response");

			INFO("");
			INFO(location.file_name() << ":" << location.line());

			CHECK(*output[0] == SuccessResponse(message.requestID));

			// now remove the first message, calls to required_messages will check what comes after the response message
			output.erase(output.begin());
		}
		else
		{
			INFO("");
			INFO(location.file_name() << ":" << location.line());

			FAIL("Expected output message of SuccessResponse with request ID " << message.requestID._val);
		}
	}

	void expect_failure(const RequestMessage& message, const std::string& error, std::source_location location = std::source_location::current())
	{
		output.clear();

		api.process_packet(message, output);

		// check output for a failure message for the request ID
		if (!output.empty())
		{
			INFO("Only message sent should be the failure response");

			INFO("");
			INFO(location.file_name() << ":" << location.line());

			CHECK(*output[0] == FailureResponse(message.requestID, error));

			REQUIRE(output.size() == 1);
		}
		else
		{
			INFO("");
			INFO(location.file_name() << ":" << location.line());

			FAIL("Expected output message of FailureResponse with request ID " << message.requestID._val << " and error: " << error);
		}
	}

	void required_messages(const std::vector<Message*>& messages, std::source_location location = std::source_location::current())
	{
		// expected message sequence:
		// m1
		// m2
		// m3
		//
		// but found:
		// m2
		// m3

		bool match = output.size() == messages.size();

		for (std::size_t i = 0; i < output.size() && match; i++)
		{
			match &= *output[i] == *messages[i];
		}

		if (!match)
		{
			UNSCOPED_INFO("Expected message sequence:");
			UNSCOPED_INFO("");

			int count = 1;

			for (auto&& msg : messages)
			{
				UNSCOPED_INFO(count << ". " << *msg);
				count++;
			}
			
			UNSCOPED_INFO("");
			UNSCOPED_INFO("Found message sequence:");
			UNSCOPED_INFO("");
			
			count = 1;

			for (auto&& msg : output)
			{
				UNSCOPED_INFO(count << ". " << *msg);
				count++;
			}
		}

		INFO("");
		INFO(location.file_name() << ":" << location.line());

		CHECK(match);
	}

private:
	RequestID m_next_request_id = RequestID(1);
	RequestID m_prev_request_id = RequestID(1);
};

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

	SECTION("Failure - Parent Task Does Not Exist")
	{
		helper.expect_failure(CreateTaskMessage(TaskID(2), helper.next_request_id(), "this is a test"), "Task with ID 2 does not exist.");

		// verify that the task was not created
		helper.expect_failure(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(2)), "Task with ID 2 does not exist.");
	}

	SECTION("Failure - Parent Is Finished")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(FinishTaskMessage(TaskID(1), helper.next_request_id()));
		helper.expect_failure(CreateTaskMessage(TaskID(1), helper.next_request_id(), "test 2"), "Cannot add sub-task. Task with ID 1 is finished.");

		// verify that the task was not created
		helper.expect_failure(TaskMessage(PacketType::REQUEST_TASK, helper.next_request_id(), TaskID(2)), "Task with ID 2 does not exist.");
	}

	SECTION("Persist")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "create 1 0 1737344939870 (this is a test)\n");
	}
}

TEST_CASE("Start Task", "[api][task]")
{
	TestHelper helper;

	SECTION("Success")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 1"));
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test 2"));

		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));

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

		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));
		helper.expect_success(StartTaskMessage(TaskID(2), helper.next_request_id()));

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
		helper.expect_failure(StartTaskMessage(TaskID(1), helper.next_request_id()), "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Task Is Already Active")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));

		helper.expect_failure(StartTaskMessage(TaskID(1), helper.next_request_id()), "Task with ID 1 is already active.");

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
		helper.expect_success(FinishTaskMessage(TaskID(1), helper.next_request_id()));

		helper.expect_failure(StartTaskMessage(TaskID(1), helper.next_request_id()), "Task with ID 1 is finished.");

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

		helper.clear_output();

		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "start 1 1737346739870\n");
	}
}

TEST_CASE("Stop Task", "[api][task]")
{
	TestHelper helper;

	SECTION("Success - Stop Active Task")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));

		helper.expect_success(StopTaskMessage(TaskID(1), helper.next_request_id()));

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
		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));
		helper.expect_success(StopTaskMessage(TaskID(1), helper.next_request_id())); // sotp the task. this step is critical to the test
		helper.expect_success(FinishTaskMessage(TaskID(1), helper.next_request_id())); // finish the task to force it out of INACTIVE state

		helper.expect_success(StartTaskMessage(TaskID(2), helper.next_request_id()));

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
		helper.expect_failure(StopTaskMessage(TaskID(1), helper.next_request_id()), "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Task Is Not Active")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));

		helper.expect_failure(StopTaskMessage(TaskID(1), helper.next_request_id()), "Task with ID 1 is not active.");

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
		helper.expect_success(FinishTaskMessage(TaskID(1), helper.next_request_id()));

		helper.expect_failure(StartTaskMessage(TaskID(1), helper.next_request_id()), "Task with ID 1 is finished.");

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
		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));

		helper.clear_output();

		helper.expect_success(StopTaskMessage(TaskID(1), helper.next_request_id()));

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
		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));

		helper.expect_success(FinishTaskMessage(TaskID(1), helper.next_request_id()));

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
			helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));

			helper.expect_success(FinishTaskMessage(TaskID(2), helper.next_request_id()));

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
			helper.expect_success(FinishTaskMessage(TaskID(2), helper.next_request_id()));

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
		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));
		helper.expect_success(FinishTaskMessage(TaskID(1), helper.next_request_id()));

		helper.expect_success(StartTaskMessage(TaskID(2), helper.next_request_id()));

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
		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));
		helper.expect_success(FinishTaskMessage(TaskID(2), helper.next_request_id()));

		helper.expect_success(StartTaskMessage(TaskID(3), helper.next_request_id()));

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
		helper.expect_failure(FinishTaskMessage(TaskID(1), helper.next_request_id()), "Task with ID 1 does not exist.");
	}

	SECTION("Failure - Task Is Already Finished")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "test"));
		helper.expect_success(FinishTaskMessage(TaskID(1), helper.next_request_id()));

		helper.expect_failure(FinishTaskMessage(TaskID(1), helper.next_request_id()), "Task with ID 1 is already finished.");
	}

	// TODO we'll have some special cases about not finishing parents before children

	SECTION("Persist")
	{
		helper.expect_success(CreateTaskMessage(NO_PARENT, helper.next_request_id(), "this is a test"));
		helper.expect_success(StartTaskMessage(TaskID(1), helper.next_request_id()));

		helper.clear_output();

		helper.expect_success(FinishTaskMessage(TaskID(1), helper.next_request_id()));

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(helper.fileOutput.str() == "finish 1 1737348539870\n");
	}
}

// TODO for now we're just going to do this in one big test
TEST_CASE("Persist Tasks", "[api][task]")
{
	TestClock clock;
	std::istringstream fileInput;
	std::ostringstream fileOutput;
	API api(clock, fileInput, fileOutput);
	std::vector<std::unique_ptr<Message>> output;

	auto create_task_1 = CreateTaskMessage(NO_PARENT, RequestID(1), "task 1");
	auto create_task_2 = CreateTaskMessage(TaskID(1), RequestID(2), "task 2");
	auto create_task_3 = CreateTaskMessage(TaskID(2), RequestID(3), "task 3");
	auto create_task_4 = CreateTaskMessage(TaskID(2), RequestID(4), "task 4");
	auto create_task_5 = CreateTaskMessage(TaskID(3), RequestID(5), "task 5");
	auto create_task_6 = CreateTaskMessage(TaskID(4), RequestID(6), "task 6");

	auto start_task_1 = StartTaskMessage(TaskID(1), RequestID(7));
	auto start_task_2 = StartTaskMessage(TaskID(2), RequestID(8));
	auto start_task_3 = StartTaskMessage(TaskID(3), RequestID(9));
	auto start_task_4 = StartTaskMessage(TaskID(4), RequestID(10));
	auto start_task_5 = StartTaskMessage(TaskID(5), RequestID(11));
	auto start_task_6 = StartTaskMessage(TaskID(6), RequestID(12));

	auto stop_task_2 = StopTaskMessage(TaskID(2), RequestID(13));
	auto stop_task_3 = StopTaskMessage(TaskID(3), RequestID(14));
	auto stop_task_4 = StopTaskMessage(TaskID(4), RequestID(15));

	auto finish_task_2 = FinishTaskMessage(TaskID(2), RequestID(16));
	auto finish_task_3 = FinishTaskMessage(TaskID(3), RequestID(17));
	auto finish_task_4 = FinishTaskMessage(TaskID(4), RequestID(18));

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

	std::ostringstream expected;
	expected << "create 1 0 1737344939870 (task 1)\n";
	expected << "create 2 1 1737346739870 (task 2)\n";
	expected << "start 2 1737348539870\n";
	expected << "create 3 2 1737350339870 (task 3)\n";
	expected << "stop 2 1737352139870\n";
	expected << "start 3 1737353939870\n";
	expected << "create 4 2 1737355739870 (task 4)\n";
	expected << "create 5 3 1737357539870 (task 5)\n";
	expected << "stop 3 1737359339870\n";
	expected << "create 6 4 1737361139870 (task 6)\n";
	expected << "start 2 1737362939870\n";
	expected << "stop 2 1737364739870\n";
	expected << "start 4 1737366539870\n";
	expected << "finish 4 1737368339870\n";
	expected << "finish 3 1737370139870\n";
	expected << "finish 2 1737371939870\n";
	expected << "start 6 1737373739870\n";
	expected << "start 5 1737376439870\n";
	expected << "start 1 1737379139870\n";

	CHECK(fileOutput.str() == expected.str());
}

TEST_CASE("Reload Tasks From File", "[api]")
{
	std::istringstream fileInput;
	std::ostringstream fileOutput;

	fileOutput << "create 1 0 1737344939870 (task 1)\n";
	fileOutput << "create 2 1 1737346739870 (task 2)\n";
	fileOutput << "start 2 1737348539870\n";
	fileOutput << "create 3 2 1737350339870 (task 3)\n";
	fileOutput << "stop 2 1737352139870\n";
	fileOutput << "start 3 1737353939870\n";
	fileOutput << "create 4 2 1737355739870 (task 4)\n";
	fileOutput << "create 5 3 1737357539870 (task 5)\n";
	fileOutput << "stop 3 1737359339870\n";
	fileOutput << "create 6 4 1737361139870 (task 6)\n";
	fileOutput << "start 2 1737362939870\n";
	fileOutput << "stop 2 1737364739870\n";
	fileOutput << "start 4 1737366539870\n";
	fileOutput << "finish 4 1737368339870\n";
	fileOutput << "finish 3 1737370139870\n";
	fileOutput << "finish 2 1737371939870\n";
	fileOutput << "start 6 1737373739870\n";
	fileOutput << "start 5 1737376439870\n";
	fileOutput << "start 1 1737379139870\n";

	fileInput = std::istringstream(fileOutput.str());
	fileOutput.clear();

	TestClock clock;
	API api(clock, fileInput, fileOutput);

	std::vector<std::unique_ptr<Message>> output;

	// now that we're setup, request the configuration and check the output
	api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, output);

	REQUIRE(output.size() == 7);

	auto task1 = TaskInfoMessage(TaskID(1), NO_PARENT, "task 1");
	auto task2 = TaskInfoMessage(TaskID(2), TaskID(1), "task 2");
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

	task1.times.emplace_back(std::chrono::milliseconds(1737379139870));
	task2.times.emplace_back(std::chrono::milliseconds(1737348539870), std::chrono::milliseconds(1737352139870));
	task2.times.emplace_back(std::chrono::milliseconds(1737362939870), std::chrono::milliseconds(1737364739870));
	task3.times.emplace_back(std::chrono::milliseconds(1737353939870), std::chrono::milliseconds(1737359339870));
	task4.times.emplace_back(std::chrono::milliseconds(1737366539870), std::chrono::milliseconds(1737368339870));
	task5.times.emplace_back(std::chrono::milliseconds(1737376439870), std::chrono::milliseconds(1737379139870));
	task6.times.emplace_back(std::chrono::milliseconds(1737373739870), std::chrono::milliseconds(1737376439870));

	task2.finishTime = std::chrono::milliseconds(1737371939870);
	task3.finishTime = std::chrono::milliseconds(1737370139870);
	task4.finishTime = std::chrono::milliseconds(1737368339870);

	verify_message(task1, *output[0]);
	verify_message(task2, *output[1]);
	verify_message(task3, *output[2]);
	verify_message(task4, *output[3]);
	verify_message(task5, *output[4]);
	verify_message(task6, *output[5]);

	verify_message(BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *output[6]);
}

TEST_CASE("request configuration at startup", "[api]")
{
	TestClock clock;
	std::istringstream fileInput;
	std::ostringstream fileOutput;
	API api(clock, fileInput, fileOutput);

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

	output.clear();

	// now that we're setup, request the configuration and check the output
	api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION }, output);

	REQUIRE(output.size() == 7);
	
	verify_message(TaskInfoMessage(TaskID(1), NO_PARENT, "task 1", std::chrono::milliseconds(1737344039870)), *output[0]);
	verify_message(TaskInfoMessage(TaskID(2), TaskID(1), "task 2", std::chrono::milliseconds(1737345839870)), *output[1]);
	verify_message(TaskInfoMessage(TaskID(3), TaskID(2), "task 3", std::chrono::milliseconds(1737347639870)), *output[2]);
	verify_message(TaskInfoMessage(TaskID(4), TaskID(2), "task 4", std::chrono::milliseconds(1737349439870)), *output[3]);
	verify_message(TaskInfoMessage(TaskID(5), TaskID(3), "task 5", std::chrono::milliseconds(1737351239870)), *output[4]);
	verify_message(TaskInfoMessage(TaskID(6), TaskID(4), "task 6", std::chrono::milliseconds(1737353039870)), *output[5]);
	
	verify_message(BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *output[6]);
}
