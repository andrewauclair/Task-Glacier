#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "lib.hpp"
#include "packets.hpp"

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

TEST_CASE("successfully create a list", "[test]")
{
	std::vector<MessageTypes> output;

	auto api = API(output);


	auto create_list = CreateListMessage(ROOT_GROUP_ID, RequestID(1), "test");

	api.process_packet(create_list);

	REQUIRE(output.size() == 1);

	if (std::holds_alternative<SuccessResponse>(output[0]))
	{
		CHECK(std::get<SuccessResponse>(output[0]).requestID == RequestID(1));
	}
	else
	{
		FAIL();
	}
}

TEST_CASE("fail to create a list", "[test]")
{
	std::vector<MessageTypes> output;

	auto api = API(output);


	auto create_list = CreateListMessage(GroupID(2), RequestID(1), "test");

	api.process_packet(create_list);

	REQUIRE(output.size() == 1);

	if (std::holds_alternative<FailureResponse>(output[0]))
	{
		CHECK(std::get<FailureResponse>(output[0]).message == "Group with ID 2 does not exist.");
	}
	else
	{
		FAIL();
	}
}