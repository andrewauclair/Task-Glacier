#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "lib.hpp"
#include "packets.hpp"

#include <vector>

template<typename T>
void verify_message(const T& expected, const Message& actual)
{
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
	API api;
	std::vector<std::unique_ptr<Message>> output;

	auto create_list = CreateListMessage(ROOT_GROUP_ID, RequestID(1), "test");
	api.process_packet(create_list, output);

	SECTION("success")
	{
		auto create_task = CreateTaskMessage(ListID(1), RequestID(2), "this is a test");

		api.process_packet(create_task, output);

		REQUIRE(output.size() == 2);

		verify_message(SuccessResponse{ RequestID(2) }, *output[1]);
	}

	SECTION("failure")
	{
		auto create_task = CreateTaskMessage(ListID(2), RequestID(2), "this is a test");

		api.process_packet(create_task, output);

		REQUIRE(output.size() == 2);

		verify_message(FailureResponse{ RequestID(2), "List with ID 2 does not exist." }, *output[1]);
	}
}

TEST_CASE("create list", "[api][list]")
{
	API api;
	std::vector<std::unique_ptr<Message>> output;

	SECTION("success")
	{
		auto create_list = CreateListMessage(ROOT_GROUP_ID, RequestID(1), "test");

		api.process_packet(create_list, output);

		REQUIRE(output.size() == 1);

		verify_message(SuccessResponse{ RequestID(1) }, *output[0]);

		// TODO use some future packets to retrieve the list to verify it exists
	}

	SECTION("failure")
	{
		auto create_list = CreateListMessage(GroupID(2), RequestID(1), "test");

		api.process_packet(create_list, output);

		REQUIRE(output.size() == 1);

		verify_message(FailureResponse(RequestID(1), "Group with ID 2 does not exist."), *output[0]);

		// TODO use some future packets to attempt to retrieve the list and verify it does not exist
	}
}

TEST_CASE("create group", "[api][group]")
{
	API api;
	std::vector<std::unique_ptr<Message>> output;

	SECTION("success")
	{
		auto create_group = CreateGroupMessage(ROOT_GROUP_ID, RequestID(1), "test");

		api.process_packet(create_group, output);

		REQUIRE(output.size() == 1);

		verify_message(SuccessResponse{ RequestID(1) }, *output[0]);

		// TODO use some future packets to retrieve the group to verify it exists
	}

	SECTION("failure")
	{
		auto create_group = CreateGroupMessage(GroupID(2), RequestID(1), "test");

		api.process_packet(create_group, output);

		REQUIRE(output.size() == 1);

		verify_message(FailureResponse(RequestID(1), "Group with ID 2 does not exist."), *output[0]);

		// TODO use some future packets to attempt to retrieve the group and verify it does not exist
	}
}

TEST_CASE("request configuration at startup", "[api]")
{
	API api;
	std::vector<std::unique_ptr<Message>> output;

	auto create_group_in_root = CreateGroupMessage(ROOT_GROUP_ID, RequestID(1), "group_one");
	auto create_list_in_root = CreateListMessage(ROOT_GROUP_ID, RequestID(2), "list_one");
	auto create_group_in_one = CreateGroupMessage(GroupID(1), RequestID(3), "group_two");
	auto create_list_in_one = CreateListMessage(GroupID(1), RequestID(4), "list_two");
	auto create_list_in_two = CreateListMessage(GroupID(2), RequestID(5), "list_three");

	// add 1 task to list one, 2 tasks to list two and 3 tasks to list three
	auto create_task_1 = CreateTaskMessage(ListID(1), RequestID(6), "task 1");
	auto create_task_2 = CreateTaskMessage(ListID(2), RequestID(7), "task 2");
	auto create_task_3 = CreateTaskMessage(ListID(2), RequestID(8), "task 3");
	auto create_task_4 = CreateTaskMessage(ListID(3), RequestID(9), "task 4");
	auto create_task_5 = CreateTaskMessage(ListID(3), RequestID(10), "task 5");
	auto create_task_6 = CreateTaskMessage(ListID(3), RequestID(11), "task 6");

	api.process_packet(create_group_in_root, output);
	api.process_packet(create_list_in_root, output);
	api.process_packet(create_group_in_one, output);
	api.process_packet(create_list_in_one, output);
	api.process_packet(create_list_in_two, output);

	api.process_packet(create_task_1, output);
	api.process_packet(create_task_2, output);
	api.process_packet(create_task_3, output);
	api.process_packet(create_task_4, output);
	api.process_packet(create_task_5, output);
	api.process_packet(create_task_6, output);

	output.clear();

	// now that we're setup, request the configuration and check the output
	api.process_packet(EmptyMessage{ PacketType::REQUEST_CONFIGURATION }, output);

	REQUIRE(output.size() == 13);
	
	verify_message(GroupInfoMessage(ROOT_GROUP_ID, ""), *output[0]);
	verify_message(GroupInfoMessage(GroupID(1), "group_one"), *output[1]);
	verify_message(GroupInfoMessage(GroupID(2), "group_two"), *output[2]);
	verify_message(ListInfoMessage(GroupID(2), ListID(3), "list_three"), *output[3]);
	verify_message(TaskInfoMessage(TaskID(4), ListID(3), "task 4"), *output[4]);
	verify_message(TaskInfoMessage(TaskID(5), ListID(3), "task 5"), *output[5]);
	verify_message(TaskInfoMessage(TaskID(6), ListID(3), "task 6"), *output[6]);
	verify_message(ListInfoMessage(GroupID(1), ListID(2), "list_two"), *output[7]);
	verify_message(TaskInfoMessage(TaskID(2), ListID(2), "task 2"), *output[8]);
	verify_message(TaskInfoMessage(TaskID(3), ListID(2), "task 3"), *output[9]);
	verify_message(ListInfoMessage(ROOT_GROUP_ID, ListID(1), "list_one"), *output[10]);
	verify_message(TaskInfoMessage(TaskID(1), ListID(1), "task 1"), *output[11]);
	
	verify_message(EmptyMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE), *output[12]);
}
