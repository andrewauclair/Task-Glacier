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
	auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
	create_task.labels = { "one", "two" };
	create_task.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

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
			auto create_task2 = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			create_task2.labels = { "one", "two" };
			create_task2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };
			CAPTURE(create_task2);

			const Message* message = &create_task2;

			CHECK(create_task == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		auto create_task2 = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
		create_task2.labels = { "one", "two" };
		create_task2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };
		CAPTURE(create_task2);

		CHECK(create_task == create_task2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		auto create_task2 = CreateTaskMessage(TaskID(15), RequestID(10), "this is a test");
		create_task2.labels = { "one", "two" };
		create_task2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };
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

		create_task2.name = create_task.name;
		create_task2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(3), TimeCodeID(3)} };
		CAPTURE(create_task2);

		CHECK(!(create_task == create_task2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		create_task.print(ss);

		auto expected_text = "CreateTaskMessage { packetType: 3, requestID: 10, parentID: 5, name: \"this is a test\", labels { \"one\", \"two\", }, timeCodes: [ [ 1 2 ], [ 2 3 ], ] }";

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
		auto verifier = PacketVerifier(create_task.pack(), 66);

		verifier
			.verify_value<std::uint32_t>(66, "packet length")
			.verify_value<std::uint32_t>(3, "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::uint32_t>(5, "parent ID")
			.verify_string("this is a test", "task name")
			.verify_value<std::int32_t>(2, "label count")
			.verify_string("one", "first label")
			.verify_string("two", "second label")
			.verify_value<std::int32_t>(2, "time code count")
			.verify_value<std::int32_t>(1, "time code 1")
			.verify_value<std::int32_t>(2, "time code 2");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<CreateTaskMessage>(create_task, 66);
	}
}

TEST_CASE("Update Task", "[message]")
{
	auto update_task = UpdateTaskMessage(RequestID(10), TaskID(5), TaskID(1), "this is a test");
	update_task.labels = { "one", "two" };
	update_task.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };

	CAPTURE(update_task);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto task = TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(1));
			CAPTURE(task);

			const Message* message = &task;

			CHECK(!(update_task == *message));
		}

		SECTION("Match")
		{
			auto update_task2 = UpdateTaskMessage(RequestID(10), TaskID(5), TaskID(1), "this is a test");
			update_task2.labels = { "one", "two" };
			update_task2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };
			CAPTURE(update_task2);

			const Message* message = &update_task2;

			CHECK(update_task == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		auto update_task2 = UpdateTaskMessage(RequestID(10), TaskID(5), TaskID(1), "this is a test");
		update_task2.labels = { "one", "two" };
		update_task2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };
		CAPTURE(update_task2);

		CHECK(update_task == update_task2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		auto update_task2 = UpdateTaskMessage(RequestID(10), TaskID(15), TaskID(1), "this is a test");
		update_task2.labels = { "one", "two" };
		update_task2.timeEntry = std::vector{ TimeEntry{TimeCategoryID(1), TimeCodeID(2)}, TimeEntry{TimeCategoryID(2), TimeCodeID(3)} };
		CAPTURE(update_task2);

		CHECK(!(update_task == update_task2));

		update_task2.taskID = update_task.taskID;
		update_task2.requestID = RequestID(15);
		CAPTURE(update_task2);

		CHECK(!(update_task == update_task2));

		update_task2.requestID = update_task.requestID;
		update_task2.name = "another task";
		CAPTURE(update_task2);

		CHECK(!(update_task == update_task2));

		update_task2.name = update_task.name;
		update_task2.parentID = TaskID(2);
		CAPTURE(update_task2);

		CHECK(!(update_task == update_task2));

		update_task2.labels = { "one" };

		CHECK(!(update_task == update_task2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		update_task.print(ss);

		auto expected_text = "UpdateTaskMessage { packetType: 15, requestID: 10, taskID: 5, parentID: 1, serverControlled: 0, locked: 0, name: \"this is a test\", labels { \"one\", \"two\", }, timeCodes: [ [ 1 2 ], [ 2 3 ], ] }";

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << update_task;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &update_task;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(update_task.pack(), 72);

		verifier
			.verify_value<std::uint32_t>(72, "packet length")
			.verify_value<std::uint32_t>(15, "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::uint32_t>(5, "task ID")
			.verify_value<std::uint32_t>(1, "parent ID")
			.verify_value<bool>(false, "server controlled")
			.verify_value<bool>(false, "locked")
			.verify_string("this is a test", "task name")
			.verify_value<std::int32_t>(2, "label count")
			.verify_string("one", "first label")
			.verify_string("two", "second label");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<UpdateTaskMessage>(update_task, 72);
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

TEST_CASE("Time Categories Data", "[messages]")
{
	std::vector<TimeCategory> timeCategories;
	timeCategories.emplace_back(TimeCategory{ TimeCategoryID(5), "one", "one", std::vector{TimeCode{TimeCodeID(1), "a"}, TimeCode{TimeCodeID(2), "b"}}});

	const auto data = TimeEntryDataPacket(timeCategories);
	CAPTURE(data);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			CAPTURE(create_task);

			const Message* message = &create_task;

			CHECK(!(data == *message));
		}

		SECTION("Match")
		{
			auto data2 = TimeEntryDataPacket(timeCategories);
			CAPTURE(data2);

			const Message* message = &data2;

			CHECK(data == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		auto data2 = TimeEntryDataPacket(timeCategories);
		CAPTURE(data2);

		CHECK(data == data2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		std::vector<TimeCategory> timeCategories2;
		timeCategories2.emplace_back(TimeCategory{ TimeCategoryID(5), "two", "two", std::vector{TimeCode{TimeCodeID(1), "a"}, TimeCode{TimeCodeID(2), "b"}}});
		auto data2 = TimeEntryDataPacket(timeCategories2);
		CAPTURE(data2);

		CHECK(!(data == data2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		data.print(ss);

		auto expected_text = "TimeEntryDataPacket { packetType: 27, TimeCategory { id: 5, name: one, label: one, inUse: 0, taskCount: 0, archived: 0\nTimeCode { id: 1, name: a, archived: 0 }\nTimeCode { id: 2, name: b, archived: 0 }\n } }";

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << data;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &data;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(data.pack(), 62);

		verifier
			.verify_value<std::uint32_t>(62, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::TIME_ENTRY_DATA), "packet ID");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<TimeEntryDataPacket>(data, 62);
	}
}

TEST_CASE("Time Categories Modify", "[messages]")
{
	std::vector<TimeCategory> timeCategories;
	timeCategories.emplace_back(TimeCategory{ TimeCategoryID(5), "one", "one", std::vector{TimeCode{TimeCodeID(1), "a"}, TimeCode{TimeCodeID(2), "b"}}});

	const auto modify = TimeEntryModifyPacket(RequestID(10), TimeCategoryModType::ADD, timeCategories);
	CAPTURE(modify);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			CAPTURE(create_task);

			const Message* message = &create_task;

			CHECK(!(modify == *message));
		}

		SECTION("Match")
		{
			auto modify2 = TimeEntryModifyPacket(RequestID(10), TimeCategoryModType::ADD, timeCategories);
			CAPTURE(modify2);

			const Message* message = &modify2;

			CHECK(modify == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		auto modify2 = TimeEntryModifyPacket(RequestID(10), TimeCategoryModType::ADD, timeCategories);
		CAPTURE(modify2);

		CHECK(modify == modify2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		std::vector<TimeCategory> timeCategories2;
		timeCategories2.emplace_back(TimeCategory{ TimeCategoryID(5), "two", "two", std::vector{TimeCode{TimeCodeID(1), "a"}, TimeCode{TimeCodeID(2), "b"}}});
		auto modify2 = TimeEntryModifyPacket(RequestID(15), TimeCategoryModType::ADD, timeCategories2);
		CAPTURE(modify2);

		CHECK(!(modify == modify2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		modify.print(ss);

		auto expected_text = "TimeEntryModifyPacket { packetType: 28, requestID: 10, type: 0, TimeCategory { id: 5, name: one, label: one, inUse: 0, taskCount: 0, archived: 0\nTimeCode { id: 1, name: a, archived: 0 }\nTimeCode { id: 2, name: b, archived: 0 }\n } }";

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << modify;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &modify;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(modify.pack(), 55);

		verifier
			.verify_value<std::uint32_t>(55, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::TIME_ENTRY_MODIFY), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<TimeEntryModifyPacket>(modify, 55);
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

TEST_CASE("Request Daily Report", "[messages]")
{
	const auto request = RequestDailyReportMessage(RequestID(10), 2, 3, 2025);
	CAPTURE(request);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			CAPTURE(create_task);

			const Message* message = &create_task;

			CHECK(!(request == *message));
		}

		SECTION("Match")
		{
			const auto request2 = RequestDailyReportMessage(RequestID(10), 2, 3, 2025);
			CAPTURE(request2);

			const Message* message = &request2;

			CHECK(request == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		const auto request2 = RequestDailyReportMessage(RequestID(10), 2, 3, 2025);
		CAPTURE(request2);

		CHECK(request == request2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		auto request2 = RequestDailyReportMessage(RequestID(12), 2, 3, 2025);
		CAPTURE(request2);

		CHECK(!(request == request2));

		request2.requestID = request.requestID;
		request2.month = 3;
		CAPTURE(request2);

		CHECK(!(request == request2));

		request2.month = request.month;
		request2.day = 4;
		CAPTURE(request2);

		CHECK(!(request == request2));

		request2.day = request.day;
		request2.year = 2026;
		CAPTURE(request2);

		CHECK(!(request == request2));
	}

	SECTION("Print")
	{
		std::ostringstream ss;

		request.print(ss);

		auto expected_text = "RequestDailyReportMessage { packetType: 17, requestID: 10, month: 2, day: 3, year: 2025 }";

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << request;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &request;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(request.pack(), 16);

		verifier
			.verify_value<std::uint32_t>(16, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::REQUEST_DAILY_REPORT), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::int8_t>(2, "month")
			.verify_value<std::int8_t>(3, "day")
			.verify_value<std::int16_t>(2025, "year");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<RequestDailyReportMessage>(request, 16);
	}
}

TEST_CASE("Daily Report", "[messages]")
{
	const auto report = DailyReportMessage(RequestID(10));
	CAPTURE(report);

	SECTION("Compare - Through Message")
	{
		SECTION("Does Not Match Other Message")
		{
			const auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
			CAPTURE(create_task);

			const Message* message = &create_task;

			CHECK(!(report == *message));
		}

		SECTION("Match")
		{
			const auto report2 = DailyReportMessage(RequestID(10));
			CAPTURE(report2);

			const Message* message = &report2;

			CHECK(report == *message);
		}
	}

	SECTION("Compare - Directly")
	{
		const auto report2 = DailyReportMessage(RequestID(10));
		CAPTURE(report2);

		CHECK(report == report2);
	}

	SECTION("Compare Messages That Do Not Match")
	{
		auto report2 = DailyReportMessage(RequestID(12));
		CAPTURE(report2);

		CHECK(!(report == report2));

		report2.requestID = report.requestID;
		report2.report.found = true;
		CAPTURE(report2);

		CHECK(!(report == report2));

		/*
		request2.month = request.month;
		request2.day = 4;
		CAPTURE(request2);

		CHECK(!(request == request2));

		request2.day = request.day;
		request2.year = 2026;
		CAPTURE(request2);

		CHECK(!(request == request2));*/
	}

	SECTION("Print - No Report Found")
	{
		std::ostringstream ss;

		report.print(ss);

		auto expected_text = "DailyReportMessage { packetType: 16, requestID: 10, report: { found: 0, month: 0, day: 0, year: 0 }}";

		CHECK(ss.str() == expected_text);

		ss.str("");

		ss << report;

		CHECK(ss.str() == expected_text);

		ss.str("");

		const Message* message = &report;

		ss << *message;

		CHECK(ss.str() == expected_text);
	}

	SECTION("Print - Report Found")
	{
		auto newReport = DailyReportMessage(RequestID(10));
		newReport.report = { true, 2, 3, 2025 };

		CAPTURE(newReport);

		std::ostringstream ss;

		newReport.print(ss);

		auto expected_text = "DailyReportMessage { packetType: 16, requestID: 10, report: { found: 1, month: 2, day: 3, year: 2025, startTime: 0ms, endTime: 0ms\nTime Pairs {\n}\nTime Per Time Code {\n}\nTotal Time: 0ms\n}}";

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack - No Report Found")
	{
		auto newReport = DailyReportMessage(RequestID(10));
		newReport.report = { false, 2, 3, 2025 };

		auto verifier = PacketVerifier(newReport.pack(), 17);

		verifier
			.verify_value<std::uint32_t>(17, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::DAILY_REPORT), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<bool>(false, "report found")
			.verify_value<std::int8_t>(2, "month")
			.verify_value<std::int8_t>(3, "day")
			.verify_value<std::int16_t>(2025, "year");
	}

	SECTION("Pack - Report Found")
	{
		auto newReport = DailyReportMessage(RequestID(10));
		newReport.report = { true, 2, 3, 2025 };

		auto verifier = PacketVerifier(newReport.pack(), 49);

		verifier
			.verify_value<std::uint32_t>(49, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::DAILY_REPORT), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<bool>(true, "report found")
			.verify_value<std::int8_t>(2, "month")
			.verify_value<std::int8_t>(3, "day")
			.verify_value<std::int16_t>(2025, "year");
	}

	SECTION("Unpack - No Report Found")
	{
		PacketTestHelper helper;
		helper.expect_packet<DailyReportMessage>(report, 17);
	}

	SECTION("Unpack - Report Found")
	{
		auto newReport = DailyReportMessage(RequestID(10));
		newReport.report = { true, 2, 3, 2025 };

		PacketTestHelper helper;
		helper.expect_packet<DailyReportMessage>(newReport, 49);
	}
}

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
		auto verifier = PacketVerifier(message.pack(), 55);

		verifier
			.verify_value<std::uint32_t>(55, "packet length")
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
		CHECK(result.bytes_read == 55);
	}
}