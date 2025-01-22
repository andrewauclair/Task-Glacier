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

TEST_CASE("create task", "[api][task]")
{
	TestClock clock;
	std::istringstream fileInput;
	std::ostringstream fileOutput;
	API api(clock, fileInput, fileOutput);
	std::vector<std::unique_ptr<Message>> output;

	SECTION("success")
	{
		auto create_task = CreateTaskMessage(NO_PARENT, RequestID(1), "this is a test");

		api.process_packet(create_task, output);

		REQUIRE(output.size() == 1);

		verify_message(SuccessResponse{ RequestID(1) }, *output[0]);
	}

	SECTION("failure")
	{
		auto create_task = CreateTaskMessage(TaskID(2), RequestID(1), "this is a test");

		api.process_packet(create_task, output);

		REQUIRE(output.size() == 1);

		verify_message(FailureResponse{ RequestID(1), "Task with ID 2 does not exist." }, *output[0]);
	}

	SECTION("persist")
	{
		auto create_task = CreateTaskMessage(NO_PARENT, RequestID(1), "this is a test");

		api.process_packet(create_task, output);

		// TODO this is all temporary. we need something setup to use, this will have to do. persistence will just be a log of steps to rebuild our data
		CHECK(fileOutput.str() == "create 1 0 1737344039870 (this is a test)\n");
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

	auto stop_task_2 = StopTaskMessage(TaskID(2), RequestID(11));
	auto stop_task_3 = StopTaskMessage(TaskID(3), RequestID(12));
	auto stop_task_4 = StopTaskMessage(TaskID(4), RequestID(13));

	auto finish_task_2 = FinishTaskMessage(TaskID(2), RequestID(14));
	auto finish_task_3 = FinishTaskMessage(TaskID(3), RequestID(15));
	auto finish_task_4 = FinishTaskMessage(TaskID(4), RequestID(16));

	api.process_packet(create_task_1, output);
	clock.time += std::chrono::hours(1);
	
	api.process_packet(create_task_2, output);
	clock.time += std::chrono::hours(1);
	
	api.process_packet(start_task_2, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(create_task_3, output);
	clock.time += std::chrono::hours(1);
	
	api.process_packet(stop_task_2, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(start_task_3, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(create_task_4, output);
	clock.time += std::chrono::hours(1);
	
	api.process_packet(create_task_5, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(stop_task_3, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(create_task_6, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(start_task_2, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(stop_task_2, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(start_task_4, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(finish_task_4, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(finish_task_3, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(finish_task_2, output);
	clock.time += std::chrono::hours(1);

	api.process_packet(start_task_1, output);

	std::ostringstream expected;
	expected << "create 1 0 1737344039870 (task 1)\n";
	expected << "create 2 1 1737347639870 (task 2)\n";
	expected << "start 2 1737351239870\n";
	expected << "create 3 2 1737354839870 (task 3)\n";
	expected << "stop 2 1737358439870\n";
	expected << "start 3 1737362039870\n";
	expected << "create 4 2 1737365639870 (task 4)\n";
	expected << "create 5 3 1737369239870 (task 5)\n";
	expected << "stop 3 1737372839870\n";
	expected << "create 6 4 1737376439870 (task 6)\n";
	expected << "start 2 1737380039870\n";
	expected << "stop 2 1737383639870\n";
	expected << "start 4 1737387239870\n";
	expected << "finish 4 1737390839870\n";
	expected << "finish 3 1737394439870\n";
	expected << "finish 2 1737398039870\n";
	expected << "start 1 1737401639870\n";

	CHECK(fileOutput.str() == expected.str());
}

TEST_CASE("Reload Tasks From File", "[api]")
{
	std::istringstream fileInput;
	std::ostringstream fileOutput;

	fileOutput << "create 1 0 1737344039870 (task 1)\n";
	fileOutput << "create 2 1 1737347639870 (task 2)\n";
	fileOutput << "start 2 1737351239870\n";
	fileOutput << "create 3 2 1737354839870 (task 3)\n";
	fileOutput << "stop 2 1737358439870\n";
	fileOutput << "start 3 1737362039870\n";
	fileOutput << "create 4 2 1737365639870 (task 4)\n";
	fileOutput << "create 5 3 1737369239870 (task 5)\n";
	fileOutput << "stop 3 1737372839870\n";
	fileOutput << "create 6 4 1737376439870 (task 6)\n";
	fileOutput << "start 2 1737380039870\n";
	fileOutput << "stop 2 1737383639870\n";
	fileOutput << "start 4 1737387239870\n";
	fileOutput << "finish 4 1737390839870\n";
	fileOutput << "finish 3 1737394439870\n";
	fileOutput << "finish 2 1737398039870\n";
	fileOutput << "start 1 1737401639870\n";

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

	task1.createTime = std::chrono::milliseconds(1737344039870);
	task2.createTime = std::chrono::milliseconds(1737347639870);
	task3.createTime = std::chrono::milliseconds(1737354839870);
	task4.createTime = std::chrono::milliseconds(1737365639870);
	task5.createTime = std::chrono::milliseconds(1737369239870);
	task6.createTime = std::chrono::milliseconds(1737376439870);

	task1.times.emplace_back(std::chrono::milliseconds(1737401639870));
	task2.times.emplace_back(std::chrono::milliseconds(1737351239870), std::chrono::milliseconds(1737358439870));
	task2.times.emplace_back(std::chrono::milliseconds(1737380039870), std::chrono::milliseconds(1737383639870));
	task3.times.emplace_back(std::chrono::milliseconds(1737362039870), std::chrono::milliseconds(1737372839870));
	task4.times.emplace_back(std::chrono::milliseconds(1737387239870), std::chrono::milliseconds(1737390839870));

	task2.finishTime = std::chrono::milliseconds(1737398039870);
	task3.finishTime = std::chrono::milliseconds(1737394439870);
	task4.finishTime = std::chrono::milliseconds(1737390839870);

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
	verify_message(TaskInfoMessage(TaskID(2), TaskID(1), "task 2", std::chrono::milliseconds(1737344039870)), *output[1]);
	verify_message(TaskInfoMessage(TaskID(3), TaskID(2), "task 3", std::chrono::milliseconds(1737344039870)), *output[2]);
	verify_message(TaskInfoMessage(TaskID(4), TaskID(2), "task 4", std::chrono::milliseconds(1737344039870)), *output[3]);
	verify_message(TaskInfoMessage(TaskID(5), TaskID(3), "task 5", std::chrono::milliseconds(1737344039870)), *output[4]);
	verify_message(TaskInfoMessage(TaskID(6), TaskID(4), "task 6", std::chrono::milliseconds(1737344039870)), *output[5]);
	
	verify_message(BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *output[6]);
}
