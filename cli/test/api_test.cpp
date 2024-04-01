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