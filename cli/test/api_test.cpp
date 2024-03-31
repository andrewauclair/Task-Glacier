#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "api.hpp"
#include "lib.hpp"
#include "packets.hpp"

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

template<typename T>
void verify_message(const T& expected, const MessageTypes& actual)
{
	if (std::holds_alternative<T>(actual))
	{
		CHECK(std::get<T>(actual) == expected);
	}
	else
	{
		FAIL();
	}
}
TEST_CASE("successfully create a list", "[test]")
{
	std::vector<MessageTypes> output;

	auto api = API(output);


	auto create_list = CreateListMessage(ROOT_GROUP_ID, RequestID(1), "test");

	api.process_packet(create_list);

	REQUIRE(output.size() == 1);

	verify_message(SuccessResponse{ RequestID(1) }, output[0]);
}

TEST_CASE("fail to create a list", "[test]")
{
	std::vector<MessageTypes> output;

	auto api = API(output);


	auto create_list = CreateListMessage(GroupID(2), RequestID(1), "test");

	api.process_packet(create_list);

	REQUIRE(output.size() == 1);

	verify_message(FailureResponse{ RequestID(1), "Group with ID 2 does not exist." }, output[0]);
}