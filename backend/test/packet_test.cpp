#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include <libassert/assert.hpp>

#include "server.hpp"
#include "packets.hpp"

#include <source_location>

std::vector<std::byte> bytes(auto... a)
{
	return std::vector<std::byte>{ static_cast<std::byte>(a)... };
}

template<typename T>
std::vector<std::byte> convert_to_bytes(T value)
{
	std::vector<std::byte> bytes(sizeof(T));
	std::memcpy(bytes.data(), &value, sizeof(T));

	return bytes;
}

struct PacketVerifier
{
private:
	std::vector<std::byte> m_bytes;
	std::size_t m_current_pos = 0;

public:
	PacketVerifier(std::vector<std::byte> bytes, std::size_t required_length)
		: m_bytes(std::move(bytes))
	{
		REQUIRE(m_bytes.size() == required_length);
	}

	template<typename T>
	PacketVerifier& verify_value(T expected, std::string_view field_name, std::source_location location = std::source_location::current())
	{
		INFO("field: " << field_name << ", expected value: " << expected);

		INFO("");
		INFO(location.file_name() << ":" << location.line());

		CHECK_THAT(std::span(m_bytes).subspan(m_current_pos, sizeof(T)), Catch::Matchers::RangeEquals(convert_to_bytes(std::byteswap(expected))));
		m_current_pos += sizeof(T);
		return *this;
	}

	PacketVerifier& verify_string(const std::string& string, std::string_view field_name, std::source_location location = std::source_location::current())
	{
		INFO("field: " << field_name << ", expected value: " << string);

		const std::uint16_t size = string.length();
		const auto size_type_length = sizeof(size);
		
		if (m_current_pos + size >= m_bytes.size())
		{
			INFO("");
			INFO(location.file_name() << ":" << location.line());

			FAIL("not enough bytes");
		}

		INFO("");
		INFO(location.file_name() << ":" << location.line());

		CHECK_THAT(std::span(m_bytes).subspan(m_current_pos, size_type_length), Catch::Matchers::RangeEquals(bytes((size & 0xFF00) >> 8, size & 0xFF)));
		
		auto string_bytes = std::span(m_bytes).subspan(m_current_pos + size_type_length, string.length());
		
		std::string str;
		
		for (auto byte : string_bytes)
		{
			str.push_back(static_cast<char>(byte));
		}
		
		m_current_pos += size_type_length + size;

		CHECK_THAT(str, Catch::Matchers::Equals(string, Catch::CaseSensitive::Yes));

		return *this;
	}
};

struct PacketTestHelper
{
	TestClock clock;
	std::ostringstream output;
	MicroTask app = MicroTask(clock, output);

	template<typename T>
	void expect_packet(const Message& message, std::size_t size, std::source_location location = std::source_location::current())
	{
		const auto result = parse_packet(message.pack());

		INFO("");
		INFO(location.file_name() << ":" << location.line());

		REQUIRE(result.packet);

		const auto* packet = dynamic_cast<T*>(result.packet.get());

		REQUIRE(packet);

		CHECK(*packet == message);
		CHECK(result.bytes_read == size);
	}
};

TEST_CASE("Create Task", "[message]")
{
	const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
	CAPTURE(create_task);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto task = TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(1));
			CAPTURE(task);

			const Message* message = &task;

			CHECK(!(create_task == *message));
		}

		SECTION("Match")
		{
			const auto create_task2 = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			CAPTURE(create_task2);

			const Message* message = &create_task2;

			CHECK(create_task == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		const auto create_task2 = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
		CAPTURE(create_task2);

		CHECK(create_task == create_task2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		auto create_task2 = CreateTaskMessage(TaskID(15), RequestID(10), "this is a test");
		CAPTURE(create_task2);

		CHECK(!(create_task == create_task2));

		create_task2.parentID = create_task.parentID;
		create_task2.requestID = RequestID(15);
		CAPTURE(create_task2);

		CHECK(!(create_task == create_task2));

		create_task2.requestID = create_task.requestID;
		create_task2.name = "another task";
		CAPTURE(create_task2);

		CHECK(!(create_task == create_task2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		create_task.print(ss);

		auto expected_text = "CreateTaskMessage { packetType: 3, requestID: 10, parentID: 5, name: \"this is a test\" }";

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << create_task;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &create_task;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(create_task.pack(), 32);

		verifier
			.verify_value<std::uint32_t>(32, "packet length")
			.verify_value<std::uint32_t>(3, "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::uint32_t>(5, "parent ID")
			.verify_string("this is a test", "task name");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<CreateTaskMessage>(create_task, 32);
	}
}

TEST_CASE("Task", "[messages]")
{
	const auto packet_type = GENERATE(PacketType::START_TASK, PacketType::STOP_TASK, PacketType::FINISH_TASK, PacketType::REQUEST_TASK);
	CAPTURE(packet_type);

	const auto task = TaskMessage(packet_type, RequestID(10), TaskID(20));
	CAPTURE(task);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			CAPTURE(create_task);

			const Message* message = &create_task;

			CHECK(!(task == *message));
		}

		SECTION("Match")
		{
			const auto task2 = TaskMessage(packet_type, RequestID(10), TaskID(20));
			CAPTURE(task2);

			const Message* message = &task2;

			CHECK(task == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		const auto task2 = TaskMessage(packet_type, RequestID(10), TaskID(20));
		CAPTURE(task2);

		CHECK(task == task2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		auto types = std::vector{ PacketType::START_TASK, PacketType::STOP_TASK, PacketType::FINISH_TASK, PacketType::REQUEST_TASK };
		std::erase(types, packet_type);

		auto task2 = TaskMessage(types[0], RequestID(10), TaskID(20));
		CAPTURE(task2);

		CHECK(!(task == task2));

		task2 = TaskMessage(packet_type, RequestID(15), TaskID(20));
		CAPTURE(task2);

		CHECK(!(task == task2));

		task2 = TaskMessage(packet_type, RequestID(10), TaskID(25));
		CAPTURE(task2);

		CHECK(!(task == task2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		task.print(ss);

		auto expected_text = std::format("TaskMessage {{ packetType: {}, requestID: 10, taskID: 20 }}", static_cast<std::int32_t>(packet_type));

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << task;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &task;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(task.pack(), 16);

		verifier
			.verify_value<std::uint32_t>(16, "packet length")
			.verify_value(static_cast<std::int32_t>(packet_type), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::uint32_t>(20, "task ID");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<TaskMessage>(task, 16);
	}
}

TEST_CASE("Success Response", "[messages]")
{
	const auto response = SuccessResponse(RequestID(10));
	CAPTURE(response);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			CAPTURE(create_task);

			const Message* message = &create_task;

			CHECK(!(response == *message));
		}

		SECTION("Match")
		{
			const auto response2 = SuccessResponse(RequestID(10));
			CAPTURE(response2);

			const Message* message = &response2;

			CHECK(response == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		const auto response2 = SuccessResponse(RequestID(10));
		CAPTURE(response2);

		CHECK(response == response2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		auto response2 = SuccessResponse(RequestID(15));
		CAPTURE(response2);

		CHECK(!(response == response2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		response.print(ss);

		auto expected_text = "SuccessResponse { packetType: 8, requestID: 10 }";

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << response;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &response;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(response.pack(), 12);

		verifier
			.verify_value<std::uint32_t>(12, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::SUCCESS_RESPONSE), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<SuccessResponse>(response, 12);
	}
}

TEST_CASE("Failure Response", "[messages]")
{
	const auto response = FailureResponse(RequestID(10), "Task does not exist.");
	CAPTURE(response);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			CAPTURE(create_task);

			const Message* message = &create_task;

			CHECK(!(response == *message));
		}

		SECTION("Match")
		{
			const auto response2 = FailureResponse(RequestID(10), "Task does not exist.");
			CAPTURE(response2);

			const Message* message = &response2;

			CHECK(response == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		const auto response2 = FailureResponse(RequestID(10), "Task does not exist.");
		CAPTURE(response2);

		CHECK(response == response2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		auto response2 = FailureResponse(RequestID(15), "Task does not exist.");
		CAPTURE(response2);

		CHECK(!(response == response2));

		response2.message = "Task is active.";
		CAPTURE(response2);

		CHECK(!(response == response2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		response.print(ss);

		auto expected_text = "FailureResponse { packetType: 9, requestID: 10, message: \"Task does not exist.\" }";

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << response;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &response;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(response.pack(), 34);

		verifier
			.verify_value<std::uint32_t>(34, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::FAILURE_RESPONSE), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_string("Task does not exist.", "message");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<FailureResponse>(response, 34);
	}
}

// don't need to care about parsing FailureResponse atm

TEST_CASE("pack the empty packet", "[message][pack]")
{
	PacketBuilder builder;

	const auto message = BasicMessage(PacketType::REQUEST_CONFIGURATION);

	auto verifier = PacketVerifier(message.pack(), 8);

	verifier
		.verify_value<std::uint32_t>(8, "packet length")
		.verify_value<std::uint32_t>(10, "packet ID");
}

TEST_CASE("unpack the empty packet", "[message][unpack]")
{
	TestClock clock;
	std::ostringstream output;
	MicroTask app(clock, output);

	const auto message = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

	const auto result = parse_packet(message.pack());

	REQUIRE(result.packet);

	const auto packet = dynamic_cast<BasicMessage&>(*result.packet.get());

	CHECK(packet == message);
	CHECK(result.bytes_read == 8);
}

TEST_CASE("Bugzilla Info Packet", "[message]")
{
	PacketBuilder builder;
	TestClock clock;
	std::ostringstream output;
	MicroTask app(clock, output);

	const auto message = BugzillaInfoMessage("bugzilla", "aBSEFASDfOJOEFfjlsojFEF");

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(message.pack(), 43);

		verifier
			.verify_value<std::uint32_t>(43, "packet length")
			.verify_value<std::uint32_t>(13, "packet ID")
			.verify_string("bugzilla", "URL")
			.verify_string("aBSEFASDfOJOEFfjlsojFEF", "API Key");
	}

	SECTION("Unpack")
	{
		const auto result = parse_packet(message.pack());

		REQUIRE(result.packet);

		const auto packet = dynamic_cast<BugzillaInfoMessage&>(*result.packet.get());

		CHECK(packet == message);
		CHECK(result.bytes_read == 43);
	}
}