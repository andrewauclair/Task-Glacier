#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "lib.hpp"
#include "packets.hpp"

#include <vector>

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

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
TEST_CASE("successfully create a list", "[test]")
{
	API api;
	auto create_list = CreateListMessage(ROOT_GROUP_ID, RequestID(1), "test");
	std::vector<MessageTypes> output;

	api.process_packet(create_list, output);

	REQUIRE(output.size() == 1);

	verify_message(SuccessResponse{ RequestID(1) }, *output[0]);

	// TODO use some future packets to retrieve the list to verify it exists
}

TEST_CASE("fail to create a list", "[test]")
{
	API api;
	auto create_list = CreateListMessage(GroupID(2), RequestID(1), "test");
	std::vector<MessageTypes> output;

	api.process_packet(create_list, output);

	REQUIRE(output.size() == 1);

	verify_message(FailureResponse(RequestID(1), "Group with ID 2 does not exist."), *output[0]);

	// TODO use some future packets to attempt to retrieve the list and verify it does not exist
}
