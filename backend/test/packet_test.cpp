#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "server.hpp"
#include "packets.hpp"
#include "api.hpp"
#include "utils.h"

#include <cpptrace/cpptrace.hpp>

using namespace std::chrono_literals;

static const TimeCategory TEST_TIME_CATEGORY_1 = TimeCategory(TimeCategoryID(1), "Test Category 1");
static const TimeCode TEST_TIME_CODE_1 = TimeCode(TimeCodeID(2), "Two");

static const TimeCategory TEST_TIME_CATEGORY_2 = TimeCategory(TimeCategoryID(2), "Test Category 2");
static const TimeCode TEST_TIME_CODE_2 = TimeCode(TimeCodeID(3), "Three");

static const TimeEntry TEST_TIME_ENTRY_1 = TimeEntry(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
static const TimeEntry TEST_TIME_ENTRY_2 = TimeEntry(TEST_TIME_CATEGORY_2, TEST_TIME_CODE_2);

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
	PacketVerifier& verify_value(T expected, std::string_view field_name)
	{
		INFO("field: " << field_name << ", expected value: " << expected);

		INFO("");
		INFO(cpptrace::generate_trace().to_string());

		CHECK_THAT(std::span(m_bytes).subspan(m_current_pos, sizeof(T)), Catch::Matchers::RangeEquals(convert_to_bytes(std::byteswap(expected))));
		m_current_pos += sizeof(T);
		return *this;
	}

	PacketVerifier& verify_string(const std::string& string, std::string_view field_name)
	{
		INFO("field: " << field_name << ", expected value: " << string);

		const std::uint16_t size = string.length();
		const auto size_type_length = sizeof(size);
		
		if (m_current_pos + size >= m_bytes.size())
		{
			INFO("");
			INFO(cpptrace::generate_trace().to_string());

			FAIL("not enough bytes");
		}

		INFO("");
		INFO(cpptrace::generate_trace().to_string());

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
	curlTest curl;
	nullDatabase db;
	TestPacketSender sender;
	API api = API(clock, curl, db, sender);
	MicroTask app = MicroTask(api, clock, db, sender);

	template<typename T>
	void expect_packet(const Message& message, std::size_t size)
	{
		TimeCategories categories;
		categories.categories.push_back(TimeCategory(TEST_TIME_CATEGORY_1.id, TEST_TIME_CATEGORY_1.name, { TimeCode(TEST_TIME_CODE_1.id, TEST_TIME_CODE_1.name) }));
		categories.categories.push_back(TimeCategory(TEST_TIME_CATEGORY_2.id, TEST_TIME_CATEGORY_2.name, { TimeCode(TEST_TIME_CODE_2.id, TEST_TIME_CODE_2.name) }));

		const auto result = parse_packet(message.pack(), categories);

		INFO("");
		INFO(cpptrace::generate_trace().to_string());

		REQUIRE(result.packet);
		CHECK(result.bytes_read == size);

		const T* packet = dynamic_cast<T*>(result.packet.get());

		REQUIRE(packet);

		verify_message(*packet, message);
	}
};

TEST_CASE("Create Task", "[message]")
{
	auto create_task = CreateTaskMessage(TaskID(5), RequestID(10), "this is a test");
	create_task.labels = { "one", "two" };
	create_task.timeEntry = std::vector{ TEST_TIME_ENTRY_1, TEST_TIME_ENTRY_2 };

	CAPTURE(create_task);

	SECTION("Print")
	{
		std::ostringstream ss;

		create_task.print(ss);

		auto expected_text = "CreateTaskMessage { packetType: CREATE_TASK (3), requestID: 10, parentID: 5, name: \"this is a test\", labels { \"one\", \"two\", }, timeCodes: [ [ Test Category 1 (1) Two (2) ], [ Test Category 2 (2) Three (3) ], ] }";

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
	update_task.state = TaskState::ACTIVE;
	update_task.labels = { "one", "two" };
	update_task.timeEntry = std::vector{ TEST_TIME_ENTRY_1, TEST_TIME_ENTRY_2 };

	CAPTURE(update_task);

	SECTION("Print")
	{
		std::ostringstream ss;

		update_task.print(ss);

		auto expected_text = "UpdateTaskMessage { packetType: UPDATE_TASK (8), requestID: 10, taskID: 5, parentID: 1, state: 1, indexInParent: 0, serverControlled: 0, locked: 0, name: \"this is a test\", labels { \"one\", \"two\", }, timeCodes: [ [ Test Category 1 (1) Two (2) ], [ Test Category 2 (2) Three (3) ], ] }";

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
		auto verifier = PacketVerifier(update_task.pack(), 84);

		verifier
			.verify_value<std::uint32_t>(84, "packet length")
			.verify_value<std::uint32_t>(8, "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::uint32_t>(5, "task ID")
			.verify_value<std::uint32_t>(1, "parent ID")
			.verify_value<std::uint32_t>(1, "state")
			.verify_value<std::uint32_t>(0, "index in parent")
			.verify_value<bool>(false, "server controlled")
			.verify_value<bool>(false, "locked")
			.verify_string("this is a test", "task name")
			.verify_value<std::int32_t>(0, "times count")
			.verify_value<std::int32_t>(2, "label count")
			.verify_string("one", "first label")
			.verify_string("two", "second label");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<UpdateTaskMessage>(update_task, 84);
	}
}

TEST_CASE("Update Task Times", "[message]")
{
	auto update = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, RequestID(10), TaskID(5), 10ms, 20ms);
	update.sessionIndex = 2;

	SECTION("pack")
	{
		auto verifier = PacketVerifier(update.pack(), 38);

		verifier
			.verify_value<std::uint32_t>(38, "packet length")
			.verify_value<std::uint32_t>(10, "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::uint32_t>(5, "task ID")
			.verify_value<std::uint32_t>(2, "session index")
			.verify_value<std::int64_t>(10, "start time")
			.verify_value<bool>(true, "stop present")
			.verify_value<std::int64_t>(20, "stop time")
			.verify_value<bool>(false, "check for overlap");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<UpdateTaskTimesMessage>(update, 38);
	}
}

TEST_CASE("Task", "[messages]")
{
	const auto packet_type = GENERATE(PacketType::START_TASK, PacketType::STOP_TASK, PacketType::FINISH_TASK, PacketType::REQUEST_TASK);
	CAPTURE(packet_type);

	const auto task = TaskMessage(packet_type, RequestID(10), TaskID(20));
	CAPTURE(task);

	SECTION("Print")
	{
		std::ostringstream ss;

		task.print(ss);

		auto expected_text = std::format("TaskMessage {{ packetType: {} ({}), requestID: 10, taskID: 20 }}", magic_enum::enum_name(packet_type), static_cast<std::int32_t>(packet_type));

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

TEST_CASE("Task State Change", "[message]")
{
	auto change = TaskStateChange(RequestID(10), TaskID(5), TaskState::FINISHED);

	SECTION("pack")
	{
		auto verifier = PacketVerifier(change.pack(), 20);

		verifier
			.verify_value<std::uint32_t>(20, "packet length")
			.verify_value<std::uint32_t>(42, "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::uint32_t>(5, "task ID")
			.verify_value<std::uint32_t>(2, "state");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<TaskStateChange>(change, 20);
	}
}

TEST_CASE("Time Categories Data", "[messages]")
{
	std::vector<TimeCategory> timeCategories;
	timeCategories.emplace_back(TimeCategory{ TimeCategoryID(5), "one", std::vector{TimeCode{TimeCodeID(1), "a"}, TimeCode{TimeCodeID(2), "b" }}});

	const auto data = TimeEntryDataPacket(timeCategories);
	CAPTURE(data);

	SECTION("Print")
	{
		std::ostringstream ss;

		data.print(ss);

		auto expected_text = "TimeEntryDataPacket { packetType: TIME_ENTRY_DATA (30), TimeCategory { id: 5, name: one, \nTimeCode { id: 1, name: a, archived: 0 }\nTimeCode { id: 2, name: b, archived: 0 }\n } }";

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
		auto verifier = PacketVerifier(data.pack(), 41);

		verifier
			.verify_value<std::uint32_t>(41, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::TIME_ENTRY_DATA), "packet ID");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<TimeEntryDataPacket>(data, 41);
	}
}

TEST_CASE("Time Categories Modify", "[messages]")
{
	std::vector<TimeCategory> timeCategories;
	timeCategories.emplace_back(TimeCategory{ TimeCategoryID(5), "one", std::vector{TimeCode{TimeCodeID(1), "a"}, TimeCode{TimeCodeID(2), "b"}}});

	auto modify = TimeEntryModifyPacket(RequestID(10));
	modify.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "A");
	modify.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 1", false);
	modify.codes.emplace_back(TimeCategoryModType::UPDATE, 0, TimeCodeID(1), "Code 2", false);

	modify.categories.emplace_back(TimeCategoryModType::UPDATE, TimeCategoryID(3), "B");
	modify.codes.emplace_back(TimeCategoryModType::UPDATE, 1, TimeCodeID(2), "Code 3", false);
	modify.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 4", false);
	CAPTURE(modify);

	SECTION("Print")
	{
		std::ostringstream ss;

		modify.print(ss);

		auto expected_text = "TimeEntryModifyPacket { packetType: TIME_ENTRY_MODIFY (31), requestID: 10, {\n    type: 0, id: 0, name: A\n    type: 1, id: 3, name: B\n}\n, {\n    type: 0, cat index: 0, code id: 0, name: Code 1, archived: 0\n    type: 1, cat index: 0, code id: 1, name: Code 2, archived: 0\n    type: 1, cat index: 1, code id: 2, name: Code 3, archived: 0\n    type: 0, cat index: 1, code id: 0, name: Code 4, archived: 0\n}\n }";

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
		auto verifier = PacketVerifier(modify.pack(), 126);

		verifier
			.verify_value<std::uint32_t>(126, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::TIME_ENTRY_MODIFY), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID");
	}

	SECTION("Unpack")
	{
		PacketTestHelper helper;
		helper.expect_packet<TimeEntryModifyPacket>(modify, 126);
	}
}

TEST_CASE("Success Response", "[messages]")
{
	const auto response = SuccessResponse(RequestOrigin{ PacketType::START_TASK, RequestID(10) });
	CAPTURE(response);

	SECTION("Print")
	{
		std::ostringstream ss;

		response.print(ss);

		auto expected_text = "SuccessResponse { packetType: SUCCESS_RESPONSE (13), request: 10 (START_TASK) }";

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
	const auto response = FailureResponse(RequestOrigin{ PacketType::START_TASK, RequestID(10) }, "Task does not exist.");
	CAPTURE(response);

	SECTION("Print")
	{
		std::ostringstream ss;

		response.print(ss);

		auto expected_text = "FailureResponse { packetType: FAILURE_RESPONSE (14), request: 10 (START_TASK), message: \"Task does not exist.\" }";

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

	SECTION("Print")
	{
		std::ostringstream ss;

		request.print(ss);

		auto expected_text = "RequestDailyReportMessage { packetType: REQUEST_DAILY_REPORT (21), requestID: 10, month: 2, day: 3, year: 2025 }";

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
	const auto report = DailyReportMessage(RequestOrigin{ PacketType::REQUEST_DAILY_REPORT, RequestID(10) }, 1737344039870ms);
	CAPTURE(report);

	SECTION("Print - No Report Found")
	{
		std::ostringstream ss;

		report.print(ss);

		auto expected_text = "DailyReportMessage { packetType: DAILY_REPORT (20), request: 10 (REQUEST_DAILY_REPORT), reportTime: 01/20/25 03:33:59.8700000, report: { found: 0, month: 0, day: 0, year: 0 }}";

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
		auto newReport = DailyReportMessage(RequestOrigin{ PacketType::REQUEST_DAILY_REPORT, RequestID(10) }, 1737344039870ms);
		newReport.report = { true, 2, 3, 2025 };

		CAPTURE(newReport);

		std::ostringstream ss;

		newReport.print(ss);

		auto expected_text = "DailyReportMessage { packetType: DAILY_REPORT (20), request: 10 (REQUEST_DAILY_REPORT), reportTime: 01/20/25 03:33:59.8700000, report: { found: 1, month: 2, day: 3, year: 2025, startTime: 0ms, endTime: 0ms\nTime Pairs {\n}\nTime Per Time Code {\n}\nTotal Time: 0ms\n}}";

		CHECK(ss.str() == expected_text);
	}

	SECTION("Pack - No Report Found")
	{
		auto newReport = DailyReportMessage(RequestOrigin{ PacketType::REQUEST_DAILY_REPORT, RequestID(10) }, 1737344039870ms);
		newReport.report = { false, 2, 3, 2025 };

		auto verifier = PacketVerifier(newReport.pack(), 25);

		verifier
			.verify_value<std::uint32_t>(25, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::DAILY_REPORT), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::int64_t>(1737344039870, "report time")
			.verify_value<bool>(false, "report found")
			.verify_value<std::int8_t>(2, "month")
			.verify_value<std::int8_t>(3, "day")
			.verify_value<std::int16_t>(2025, "year");
	}

	SECTION("Pack - Report Found")
	{
		auto newReport = DailyReportMessage(RequestOrigin{ PacketType::REQUEST_DAILY_REPORT, RequestID(10) }, 1737344039870ms);
		newReport.report = { true, 2, 3, 2025 };

		auto verifier = PacketVerifier(newReport.pack(), 57);

		verifier
			.verify_value<std::uint32_t>(57, "packet length")
			.verify_value(static_cast<std::int32_t>(PacketType::DAILY_REPORT), "packet ID")
			.verify_value<std::uint32_t>(10, "request ID")
			.verify_value<std::int64_t>(1737344039870, "report time")
			.verify_value<bool>(true, "report found")
			.verify_value<std::int8_t>(2, "month")
			.verify_value<std::int8_t>(3, "day")
			.verify_value<std::int16_t>(2025, "year");
	}

	SECTION("Unpack - No Report Found")
	{
		PacketTestHelper helper;
		helper.expect_packet<DailyReportMessage>(report, 25);
	}

	SECTION("Unpack - Report Found")
	{
		auto newReport = DailyReportMessage(RequestOrigin{ PacketType::REQUEST_DAILY_REPORT, RequestID(10) }, 1737344039870ms);
		newReport.report = { true, 2, 3, 2025 };

		PacketTestHelper helper;
		helper.expect_packet<DailyReportMessage>(newReport, 57);
	}
}

TEST_CASE("pack the empty packet", "[message][pack]")
{
	PacketBuilder builder;

	const auto message = BasicMessage(PacketType::REQUEST_CONFIGURATION);

	auto verifier = PacketVerifier(message.pack(), 8);

	verifier
		.verify_value<std::uint32_t>(8, "packet length")
		.verify_value<std::uint32_t>(15, "packet ID");
}

TEST_CASE("unpack the empty packet", "[message][unpack]")
{
	TestClock clock;
	curlTest curl;
	nullDatabase db;
	TestPacketSender sender;
	API api = API(clock, curl, db, sender);
	MicroTask app = MicroTask(api, clock, db, sender);

	const auto message = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

	TimeCategories categories;
	categories.categories.push_back(TimeCategory(TEST_TIME_CATEGORY_1.id, TEST_TIME_CATEGORY_1.name, { TimeCode(TEST_TIME_CODE_1.id, TEST_TIME_CODE_1.name) }));
	categories.categories.push_back(TimeCategory(TEST_TIME_CATEGORY_2.id, TEST_TIME_CATEGORY_2.name, { TimeCode(TEST_TIME_CODE_2.id, TEST_TIME_CODE_2.name) }));

	const auto result = parse_packet(message.pack(), categories);

	REQUIRE(result.packet);

	const auto packet = dynamic_cast<BasicMessage&>(*result.packet.get());

	verify_message(packet, message);
	CHECK(result.bytes_read == 8);
}

TEST_CASE("Bugzilla Info Packet", "[message]")
{
	PacketBuilder builder;
	TestClock clock;
	curlTest curl;
	nullDatabase db;
	TestPacketSender sender;
	API api = API(clock, curl, db, sender);
	MicroTask app = MicroTask(api, clock, db, sender);

	auto message = BugzillaInfoMessage(BugzillaInstanceID(1), "bugzilla", "0.0.0.0", "aBSEFASDfOJOEFfjlsojFEF");
	message.username = "admin";
	message.rootTaskID = TaskID(3);
	message.groupTasksBy.push_back("product");
	message.groupTasksBy.push_back("severity");
	message.labelToField["Priority"] = "priority";
	message.labelToField["Status"] = "status";

	SECTION("Pack")
	{
		auto verifier = PacketVerifier(message.pack(), 130);

		verifier
			.verify_value<std::uint32_t>(130, "packet length")
			.verify_value<std::uint32_t>(18, "packet ID")
			.verify_value<std::int32_t>(1, "instance ID")
			.verify_string("bugzilla", "name")
			.verify_string("0.0.0.0", "URL")
			.verify_string("aBSEFASDfOJOEFfjlsojFEF", "API Key")
			.verify_string("admin", "username")
			.verify_value<std::int32_t>(3, "root task ID")
			.verify_value<std::int32_t>(2, "group task by count")
			.verify_string("product", " group task by 1")
			.verify_string("severity", "group task by 2")
			.verify_value<std::int32_t>(2, "label to field count")
			.verify_string("Priority", "label 1")
			.verify_string("priority", "field 1")
			.verify_string("Status", "label 2")
			.verify_string("status", "field 2");
	}

	SECTION("Unpack")
	{
		TimeCategories categories;
		categories.categories.push_back(TimeCategory(TEST_TIME_CATEGORY_1.id, TEST_TIME_CATEGORY_1.name, { TimeCode(TEST_TIME_CODE_1.id, TEST_TIME_CODE_1.name) }));
		categories.categories.push_back(TimeCategory(TEST_TIME_CATEGORY_2.id, TEST_TIME_CATEGORY_2.name, { TimeCode(TEST_TIME_CODE_2.id, TEST_TIME_CODE_2.name) }));

		const auto result = parse_packet(message.pack(), categories);

		REQUIRE(result.packet);

		const auto packet = dynamic_cast<BugzillaInfoMessage&>(*result.packet.get());

		verify_message(packet, message);
		CHECK(result.bytes_read == 130);
	}
}