#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "server.hpp"
#include "packets.hpp"

#include <vector>

template<typename T>
void verify_message(const T& expected, const Message& actual)
{
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
	API api(clock);
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
}

TEST_CASE("request configuration at startup", "[api]")
{
	TestClock clock;
	API api(clock);
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
	
	verify_message(TaskInfoMessage(TaskID(1), NO_PARENT, "task 1"), *output[0]);
	verify_message(TaskInfoMessage(TaskID(2), TaskID(1), "task 2"), *output[1]);
	verify_message(TaskInfoMessage(TaskID(3), TaskID(2), "task 3"), *output[2]);
	verify_message(TaskInfoMessage(TaskID(4), TaskID(2), "task 4"), *output[3]);
	verify_message(TaskInfoMessage(TaskID(5), TaskID(3), "task 5"), *output[4]);
	verify_message(TaskInfoMessage(TaskID(6), TaskID(4), "task 6"), *output[5]);
	
	verify_message(BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *output[6]);
}
