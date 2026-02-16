#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "packets.hpp"
#include "database.hpp"
#include "utils.h"

#include <filesystem>

#include "SQLiteCpp/Database.h"

using namespace std::chrono_literals;

// all tests directly related to using sqlite




// create new database

// load database and pass data to other objects (we'll verify everything is loaded through a config request)

// save a task

// save time configuration data

// save task sessions

// save task time entry

// save bugzilla configuration

// save bugzilla group by

// save bugzilla bug ID to task ID

static const TimeCategory TEST_TIME_CATEGORY_UNKNOWN = TimeCategory(TimeCategoryID(0), "Unknown");
static const TimeCode TEST_TIME_CODE_UNKNOWN = TimeCode(TimeCodeID(0), "Unknown");

static const TimeCategory TEST_TIME_CATEGORY_1 = TimeCategory(TimeCategoryID(1), "Test Category 1");
static const TimeCode TEST_TIME_CODE_1 = TimeCode(TimeCodeID(2), "Two");

static const TimeCategory TEST_TIME_CATEGORY_2 = TimeCategory(TimeCategoryID(2), "Test Category 2");
static const TimeCode TEST_TIME_CODE_2 = TimeCode(TimeCodeID(3), "Three");

static const TimeCategory TEST_TIME_CATEGORY_3 = TimeCategory(TimeCategoryID(3), "Test Category 3");
static const TimeCategory TEST_TIME_CATEGORY_4 = TimeCategory(TimeCategoryID(4), "Test Category 4");
static const TimeCategory TEST_TIME_CATEGORY_5 = TimeCategory(TimeCategoryID(5), "Test Category 5");

static const TimeEntry TEST_TIME_ENTRY_1 = TimeEntry(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
static const TimeEntry TEST_TIME_ENTRY_2 = TimeEntry(TEST_TIME_CATEGORY_2, TEST_TIME_CODE_2);

TEST_CASE("Create Database", "[database]")
{
	TestPacketSender sender;
	DatabaseImpl db(":memory:", sender);

	const auto has_table = [&](const std::string& table)
		{
			SQLite::Statement query(db.database(), "SELECT name FROM sqlite_schema WHERE type='table' AND name=?");
			query.bind(1, table);
			query.executeStep();
			INFO(table);
			CHECK(query.hasRow());
		};

	has_table("tasks");
	has_table("timeEntryCategory");
	has_table("timeEntryCode");
	has_table("timeEntryTask");
	has_table("timeEntrySession");
	has_table("bugzilla");
	has_table("bugzillaGroupBy");
	has_table("bugzillaBugToTask");
}

TEST_CASE("Load Database", "[database]")
{
	std::filesystem::remove("database_load_test.db3");

	{
		TestPacketSender sender;
		TestHelper<DatabaseImpl> helper{ DatabaseImpl("database_load_test.db3", sender) };

		auto addTimeEntry = TimeEntryModifyPacket(RequestID(1));
		addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "A");
		addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 1", true);
		addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 2", false);

		addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "B");
		addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 3", false);
		addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 4", true);

		helper.expect_success(addTimeEntry);

		CreateTaskMessage create1(NO_PARENT, RequestID(1), "parent");
		create1.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
		create1.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

		CreateTaskMessage create2(TaskID(1), RequestID(2), "child 1");
		create2.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_2);
		create2.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(3), ""));

		CreateTaskMessage create3(TaskID(2), RequestID(3), "child 2");
		create3.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_2);
		create3.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

		CreateTaskMessage create4(NO_PARENT, RequestID(3), "parent 2");
		create3.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_2);
		create3.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

		helper.expect_success(create1);
		helper.expect_success(create2);
		helper.expect_success(create3);
		helper.expect_success(create4);

		helper.expect_success(TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(2)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(3)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(1)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(2)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(3)));
		helper.expect_success(TaskMessage(PacketType::FINISH_TASK, RequestID(1), TaskID(2)));
		helper.expect_success(TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(4)));

		auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(1);
		configure.groupTasksBy.push_back("priority");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		const std::string fieldsResponse = R"bugs_data(
		{
		  "fields": [
			{
			  "display_name": "Priority",
			  "name": "priority",
			  "type": 2,
			  "is_mandatory": false,
			  "value_field": null,
			  "values": [
				{
				  "sortkey": 100,
				  "sort_key": 100,
				  "visibility_values": [],
				  "name": "P1"
				},
				{
				  "sort_key": 200,
				  "name": "P2",
				  "visibility_values": [],
				  "sortkey": 200
				},
				{
				  "sort_key": 300,
				  "visibility_values": [],
				  "name": "P3",
				  "sortkey": 300
				},
				{
				  "sort_key": 400,
				  "name": "P4",
				  "visibility_values": [],
				  "sortkey": 400
				}
			  ],
			  "visibility_values": [],
			  "visibility_field": null,
			  "is_on_bug_entry": false,
			  "is_custom": false,
			  "id": 13
			},
			{
			  "display_name": "Severity",
			  "name": "severity",
			  "type": 2,
			  "is_mandatory": false,
			  "value_field": null,
			  "values": [
				{
				  "sortkey": 100,
				  "sort_key": 100,
				  "visibility_values": [],
				  "name": "Nitpick"
				},
				{
				  "sort_key": 200,
				  "name": "Minor",
				  "visibility_values": [],
				  "sortkey": 200
				},
				{
				  "sort_key": 300,
				  "visibility_values": [],
				  "name": "Critical",
				  "sortkey": 300
				},
				{
				  "sort_key": 400,
				  "name": "Blocker",
				  "visibility_values": [],
				  "sortkey": 400
				}
			  ],
			  "visibility_values": [],
			  "visibility_field": null,
			  "is_on_bug_entry": false,
			  "is_custom": false,
			  "id": 14
			}
		  ]
		}
	)bugs_data";

		helper.curl.requestResponse.emplace_back(fieldsResponse);
		helper.curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 50, \"assigned_to\": \"test\", \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
			"{ \"id\": 55, \"assigned_to\": \"test\", \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
			"{ \"id\": 60, \"assigned_to\": \"test\", \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"P1\", \"severity\": \"Critical\" },"
			"{ \"id\": 65, \"assigned_to\": \"test\", \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"P3\", \"severity\": \"Blocker\" },"
			"{ \"id\": 70, \"assigned_to\": \"test\", \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Nitpick\" } ] }");
		helper.curl.requestResponse.push_back(helper.curl.requestResponse.back());

		helper.api.process_packet(configure);
	}

	{
		TestPacketSender sender;
		TestHelper<DatabaseImpl> helper{ DatabaseImpl("database_load_test.db3", sender) };

		helper.api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION });

		REQUIRE(helper.sender.output.size() == 22);

		// test next bugzilla instance ID
		helper.sender.output.clear();
		auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla2", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(1);
		configure.groupTasksBy.push_back("product");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		helper.curl.requestResponse.emplace_back("{ \"fields\": [] }");
		helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");
		helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");
		helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");
		helper.curl.requestResponse.emplace_back("{ \"bugs\": [] }");

		helper.api.process_packet(configure);

		REQUIRE(helper.sender.output.size() == 17);
		CHECK(dynamic_cast<BugzillaInfoMessage*>(helper.sender.output[1].get())->instanceID == BugzillaInstanceID(2));

		// test next task ID
		helper.sender.output.clear();
		helper.expect_success(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
		REQUIRE(helper.sender.output.size() == 1);
		CHECK(dynamic_cast<TaskInfoMessage*>(helper.sender.output[0].get())->taskID == TaskID(18));

		// test next time category ID
		// test next time code ID
		auto modify = TimeEntryModifyPacket(RequestID(1));
		modify.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "C");
		modify.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 5");

		helper.sender.output.clear();
		helper.api.process_packet(modify);
		REQUIRE(helper.sender.output.size() == 2);

		auto* timeEntry = dynamic_cast<TimeEntryDataPacket*>(helper.sender.output[1].get());
		REQUIRE(timeEntry->timeCategories.size() == 3);
		CHECK(timeEntry->timeCategories[2].id == TEST_TIME_CATEGORY_3.id);
		CHECK(timeEntry->timeCategories[2].codes[0].id == TimeCodeID(5));

		// archived
		CHECK(timeEntry->timeCategories[0].codes[0].archived);
		CHECK(!timeEntry->timeCategories[0].codes[1].archived);
		CHECK(!timeEntry->timeCategories[1].codes[0].archived);
		CHECK(timeEntry->timeCategories[1].codes[1].archived);

		// verify that we have an active task by starting a new task (this sends an info message for the old active task)
		helper.expect_success(TaskMessage(PacketType::START_TASK, RequestID(1), TaskID(1)));

		REQUIRE(helper.sender.output.size() == 2);
	}
}

TEST_CASE("Load Unspecified Task from Database - Active", "[database]")
{
	std::filesystem::remove("database_load_test.db3");

	{
		TestPacketSender do_not_use;
		TestHelper<DatabaseImpl> helper{ DatabaseImpl("database_load_test.db3", do_not_use) };

		CreateTaskMessage create1(NO_PARENT, RequestID(1), "Bugzilla");

		helper.expect_success(create1);

		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, RequestID(10), UNSPECIFIED_TASK));
	}

	{
		TestPacketSender do_not_use;
		TestHelper<DatabaseImpl> helper{ DatabaseImpl("database_load_test.db3", do_not_use) };

		helper.api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION });

		auto timeCategories = TimeEntryDataPacket({});
		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);
		
		auto root = TaskInfoMessage(TaskID(1), NO_PARENT, "Bugzilla");
		root.createTime = std::chrono::milliseconds(1737344039870);

		auto bulk_start = BasicMessage(PacketType::BULK_TASK_INFO_START);
		auto bulk_finish = BasicMessage(PacketType::BULK_TASK_INFO_FINISH);

		auto active_task = BasicMessage(PacketType::UNSPECIFIED_TASK_ACTIVE);

		helper.required_messages({ &timeCategories, &bulk_start, &root, &active_task, &bulk_finish, &complete });
	}
}

TEST_CASE("Load Unspecified Task from Database - Inactive", "[database]")
{
	std::filesystem::remove("database_load_test.db3");

	{
		TestPacketSender do_not_use;
		TestHelper<DatabaseImpl> helper{ DatabaseImpl("database_load_test.db3", do_not_use) };

		CreateTaskMessage create1(NO_PARENT, RequestID(1), "Bugzilla");

		helper.expect_success(create1);

		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, RequestID(10), UNSPECIFIED_TASK));
		helper.expect_success(TaskMessage(PacketType::STOP_UNSPECIFIED_TASK, RequestID(10), TaskID(1)));
	}

	{
		TestPacketSender do_not_use;
		TestHelper<DatabaseImpl> helper{ DatabaseImpl("database_load_test.db3", do_not_use) };

		helper.api.process_packet(BasicMessage{ PacketType::REQUEST_CONFIGURATION });

		auto timeCategories = TimeEntryDataPacket({});
		auto complete = BasicMessage(PacketType::REQUEST_CONFIGURATION_COMPLETE);

		auto root = TaskInfoMessage(TaskID(1), NO_PARENT, "Bugzilla");
		root.createTime = std::chrono::milliseconds(1737344039870);
		root.state = TaskState::ACTIVE;
		root.times.emplace_back(std::chrono::milliseconds(1737344939870));
		root.times.back().timeEntry.emplace_back(TEST_TIME_CATEGORY_UNKNOWN, TEST_TIME_CODE_UNKNOWN);

		auto bulk_start = BasicMessage(PacketType::BULK_TASK_INFO_START);
		auto bulk_finish = BasicMessage(PacketType::BULK_TASK_INFO_FINISH);

		helper.required_messages({ &timeCategories, &bulk_start, &root, &bulk_finish, &complete });

		// verify that we can start the unspecified task again
		helper.expect_success(TaskMessage(PacketType::START_UNSPECIFIED_TASK, helper.next_request_id(), UNSPECIFIED_TASK));
	}
}

TEST_CASE("Write Task to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	SECTION("Create")
	{
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
		api.process_packet(CreateTaskMessage(TaskID(1), RequestID(2), "this is a test"));
		
		SQLite::Statement query(database.database(), "SELECT * FROM tasks WHERE TaskID == 2");
		query.executeStep();

		REQUIRE(query.hasRow());

		int id = query.getColumn(0);
		std::string name = query.getColumn(1);
		int parentID = query.getColumn(2);
		int state = query.getColumn(3);
		std::int64_t create_time = query.getColumn(4);
		std::int64_t finish_time = query.getColumn(5);

		CHECK(id == 2);
		CHECK(name == "this is a test");
		CHECK(parentID == 1);
		CHECK(state == 0);
		CHECK(create_time == 1737344939870);
		CHECK(finish_time == 0);

		query.executeStep();
		CHECK(!query.hasRow());
	}

	SECTION("Start")
	{
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
		api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(2), TaskID(1)));

		SQLite::Statement query(database.database(), "SELECT * FROM tasks WHERE TaskID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		int id = query.getColumn(0);
		std::string name = query.getColumn(1);
		int parentID = query.getColumn(2);
		int state = query.getColumn(3);
		std::int64_t create_time = query.getColumn(4);
		std::int64_t finish_time = query.getColumn(5);

		CHECK(id == 1);
		CHECK(name == "parent");
		CHECK(parentID == 0);
		CHECK(state == 1);
		CHECK(create_time == 1737344039870);
		CHECK(finish_time == 0);

		query.executeStep();
		CHECK(!query.hasRow());

		SECTION("Write Stop When Active")
		{
			api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
			api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(2), TaskID(2)));

			query = SQLite::Statement(database.database(), "SELECT * FROM tasks WHERE TaskID == 1");
			query.executeStep();

			REQUIRE(query.hasRow());

			id = query.getColumn(0);
			name = query.getColumn(1).getString();
			parentID = query.getColumn(2);
			state = query.getColumn(3);
			create_time = query.getColumn(4);
			finish_time = query.getColumn(5);

			CHECK(id == 1);
			CHECK(name == "parent");
			CHECK(parentID == 0);
			CHECK(state == 0);
			CHECK(create_time == 1737344039870);
			CHECK(finish_time == 0);

			query.executeStep();
			CHECK(!query.hasRow());
		}
	}

	SECTION("Stop")
	{
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
		api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(2), TaskID(1)));
		api.process_packet(TaskMessage(PacketType::STOP_TASK, RequestID(3), TaskID(1)));


		SQLite::Statement query(database.database(), "SELECT * FROM tasks WHERE TaskID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		int id = query.getColumn(0);
		std::string name = query.getColumn(1);
		int parentID = query.getColumn(2);
		int state = query.getColumn(3);
		std::int64_t create_time = query.getColumn(4);
		std::int64_t finish_time = query.getColumn(5);

		CHECK(id == 1);
		CHECK(name == "parent");
		CHECK(parentID == 0);
		CHECK(state == 0);
		CHECK(create_time == 1737344039870);
		CHECK(finish_time == 0);

		query.executeStep();
		CHECK(!query.hasRow());
	}

	SECTION("Finish")
	{
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
		api.process_packet(TaskMessage(PacketType::FINISH_TASK, RequestID(2), TaskID(1)));

		SQLite::Statement query(database.database(), "SELECT * FROM tasks WHERE TaskID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		int id = query.getColumn(0);
		std::string name = query.getColumn(1);
		int parentID = query.getColumn(2);
		int state = query.getColumn(3);
		std::int64_t create_time = query.getColumn(4);
		std::int64_t finish_time = query.getColumn(5);

		CHECK(id == 1);
		CHECK(name == "parent");
		CHECK(parentID == 0);
		CHECK(state == 2);
		CHECK(create_time == 1737344039870);
		CHECK(finish_time == 1737344939870);

		query.executeStep();
		CHECK(!query.hasRow());
	}

	SECTION("Reparent")
	{
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(2), "this is a test"));

		api.process_packet(UpdateTaskMessage(RequestID(3), TaskID(2), TaskID(1), "this is a test"));

		SQLite::Statement query(database.database(), "SELECT * FROM tasks WHERE TaskID == 2");
		query.executeStep();

		REQUIRE(query.hasRow());

		int id = query.getColumn(0);
		std::string name = query.getColumn(1);
		int parentID = query.getColumn(2);
		int state = query.getColumn(3);
		std::int64_t create_time = query.getColumn(4);
		std::int64_t finish_time = query.getColumn(5);

		CHECK(id == 2);
		CHECK(name == "this is a test");
		CHECK(parentID == 1);
		CHECK(state == 0);
		CHECK(create_time == 1737344939870);
		CHECK(finish_time == 0);

		query.executeStep();
		CHECK(!query.hasRow());
	}

	SECTION("Rename")
	{
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));

		api.process_packet(UpdateTaskMessage(RequestID(3), TaskID(1), NO_PARENT, "child"));

		SQLite::Statement query(database.database(), "SELECT * FROM tasks WHERE TaskID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		int id = query.getColumn(0);
		std::string name = query.getColumn(1);
		int parentID = query.getColumn(2);
		int state = query.getColumn(3);
		std::int64_t create_time = query.getColumn(4);
		std::int64_t finish_time = query.getColumn(5);

		CHECK(id == 1);
		CHECK(name == "child");
		CHECK(parentID == 0);
		CHECK(state == 0);
		CHECK(create_time == 1737344039870);
		CHECK(finish_time == 0);

		query.executeStep();
		CHECK(!query.hasRow());
	}

	SECTION("Lock")
	{
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));

		auto update = UpdateTaskMessage(RequestID(3), TaskID(1), NO_PARENT, "parent");
		update.locked = 1;

		api.process_packet(update);

		SQLite::Statement query(database.database(), "SELECT * FROM tasks WHERE TaskID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		int locked = query.getColumn(6);

		CHECK(locked == 1);

		query.executeStep();
		CHECK(!query.hasRow());
	}

	SECTION("Index in Parent")
	{
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));
		api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));

		SQLite::Statement query(database.database(), "SELECT * FROM tasks");
		query.executeStep();

		REQUIRE(query.hasRow());

		int indexInParent = query.getColumn(8);

		CHECK(indexInParent == 0);

		query.executeStep();
		CHECK(query.hasRow());

		indexInParent = query.getColumn(8);

		CHECK(indexInParent == 1);

		query.executeStep();
		CHECK(query.hasRow());

		indexInParent = query.getColumn(8);

		CHECK(indexInParent == 2);

		query.executeStep();
		CHECK(!query.hasRow());
	}
}

TEST_CASE("Write Task Session to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	auto addTimeEntry = TimeEntryModifyPacket(RequestID(1));
	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "A");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 1", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 2", false);

	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "B");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 3", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 4", false);

	api.process_packet(addTimeEntry);

	auto create = CreateTaskMessage(NO_PARENT, RequestID(2), "parent");
	create.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
	create.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

	api.process_packet(create);
	api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(3), TaskID(1)));
	api.process_packet(TaskMessage(PacketType::STOP_TASK, RequestID(4), TaskID(1)));
	api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(5), TaskID(1)));

	SQLite::Statement query(database.database(), "SELECT * FROM timeEntrySession WHERE TaskID == 1");
	query.executeStep();

	REQUIRE(query.hasRow());

	int id = query.getColumn(0);
	int sessionIndex = query.getColumn(1);
	int timeCategoryID = query.getColumn(2);
	int timeCodeID = query.getColumn(3);
	std::int64_t start = query.getColumn(4);
	std::int64_t stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(timeCategoryID == 1);
	CHECK(timeCodeID == 2);
	CHECK(start == 1737344939870);
	CHECK(stop == 1737345839870);
	
	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	timeCodeID = query.getColumn(3);
	start = query.getColumn(4);
	stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(timeCategoryID == 2);
	CHECK(timeCodeID == 4);
	CHECK(start == 1737344939870);
	CHECK(stop == 1737345839870);

	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	timeCodeID = query.getColumn(3);
	start = query.getColumn(4);
	stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 1);
	CHECK(start == 1737346739870);
	CHECK(stop == 0);
	CHECK(timeCategoryID == 1);
	CHECK(timeCodeID == 2);

	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	timeCodeID = query.getColumn(3);
	start = query.getColumn(4);
	stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 1);
	CHECK(timeCategoryID == 2);
	CHECK(timeCodeID == 4);
	CHECK(start == 1737346739870);
	CHECK(stop == 0);

	query.executeStep();

	REQUIRE(!query.hasRow());
}

TEST_CASE("Add Session - Write to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	auto addTimeEntry = TimeEntryModifyPacket(RequestID(1));
	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "A");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 1", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 2", false);

	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "B");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 3", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 4", false);

	api.process_packet(addTimeEntry);

	auto create = CreateTaskMessage(NO_PARENT, RequestID(2), "parent");
	create.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
	create.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

	api.process_packet(create);

	auto add = UpdateTaskTimesMessage(PacketType::ADD_TASK_SESSION, RequestID(2), TaskID(1), 10000ms, 20000ms);

	api.process_packet(add);

	SQLite::Statement query(database.database(), "SELECT * FROM timeEntrySession WHERE TaskID == 1");
	query.executeStep();

	REQUIRE(query.hasRow());

	int id = query.getColumn(0);
	int sessionIndex = query.getColumn(1);
	int timeCategoryID = query.getColumn(2);
	int timeCodeID = query.getColumn(3);
	std::int64_t start = query.getColumn(4);
	std::int64_t stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(timeCategoryID == 1);
	CHECK(timeCodeID == 2);
	CHECK(start == 10000);
	CHECK(stop == 20000);

	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	timeCodeID = query.getColumn(3);
	start = query.getColumn(4);
	stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(timeCategoryID == 2);
	CHECK(timeCodeID == 4);
	CHECK(start == 10000);
	CHECK(stop == 20000);

	query.executeStep();

	REQUIRE(!query.hasRow());
}

TEST_CASE("Edit Session - Write to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	auto addTimeEntry = TimeEntryModifyPacket(RequestID(1));
	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "A");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 1", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 2", false);

	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "B");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 3", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 4", false);

	api.process_packet(addTimeEntry);

	auto create = CreateTaskMessage(NO_PARENT, RequestID(2), "parent");
	create.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
	create.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

	api.process_packet(create);
	api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(3), TaskID(1)));
	api.process_packet(TaskMessage(PacketType::STOP_TASK, RequestID(4), TaskID(1)));

	auto edit = UpdateTaskTimesMessage(PacketType::EDIT_TASK_SESSION, RequestID(1), TaskID(1), 10000ms, 20000ms);
	edit.sessionIndex = 0;

	api.process_packet(edit);

	SQLite::Statement query(database.database(), "SELECT * FROM timeEntrySession WHERE TaskID == 1");
	query.executeStep();

	REQUIRE(query.hasRow());

	int id = query.getColumn(0);
	int sessionIndex = query.getColumn(1);
	int timeCategoryID = query.getColumn(2);
	int timeCodeID = query.getColumn(3);
	std::int64_t start = query.getColumn(4);
	std::int64_t stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(timeCategoryID == 1);
	CHECK(timeCodeID == 2);
	CHECK(start == 10000);
	CHECK(stop == 20000);

	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	timeCodeID = query.getColumn(3);
	start = query.getColumn(4);
	stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(timeCategoryID == 2);
	CHECK(timeCodeID == 4);
	CHECK(start == 10000);
	CHECK(stop == 20000);

	query.executeStep();

	REQUIRE(!query.hasRow());
}

TEST_CASE("Remove Session - Write to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	auto addTimeEntry = TimeEntryModifyPacket(RequestID(1));
	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "A");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 1", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 2", false);

	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "B");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 3", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 4", false);

	api.process_packet(addTimeEntry);

	auto create = CreateTaskMessage(NO_PARENT, RequestID(2), "parent");
	create.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
	create.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

	api.process_packet(create);
	api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(3), TaskID(1)));
	api.process_packet(TaskMessage(PacketType::STOP_TASK, RequestID(4), TaskID(1)));
	api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(5), TaskID(1)));

	auto remove = UpdateTaskTimesMessage(PacketType::REMOVE_TASK_SESSION, RequestID(1), TaskID(1), 10000ms, 20000ms);
	remove.sessionIndex = 0;

	api.process_packet(remove);

	SQLite::Statement query(database.database(), "SELECT * FROM timeEntrySession WHERE TaskID == 1");
	query.executeStep();

	REQUIRE(query.hasRow());

	int id = query.getColumn(0);
	int sessionIndex = query.getColumn(1);
	int timeCategoryID = query.getColumn(2);
	int timeCodeID = query.getColumn(3);
	std::int64_t start = query.getColumn(4);
	std::int64_t stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(start == 1737346739870);
	CHECK(stop == 0);
	CHECK(timeCategoryID == 1);
	CHECK(timeCodeID == 2);

	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	timeCodeID = query.getColumn(3);
	start = query.getColumn(4);
	stop = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(timeCategoryID == 2);
	CHECK(timeCodeID == 4);
	CHECK(start == 1737346739870);
	CHECK(stop == 0);

	query.executeStep();

	REQUIRE(!query.hasRow());
}

TEST_CASE("Write Time Configuration to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	auto addTimeEntry = TimeEntryModifyPacket(RequestID(1));
	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "A");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 1", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 2", false);

	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "B");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 3", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 4", false);

	api.process_packet(addTimeEntry);

	SECTION("Add")
	{
		SQLite::Statement query(database.database(), "SELECT * FROM timeEntryCategory");
		query.executeStep();

		REQUIRE(query.hasRow());

		int categoryID = query.getColumn(0);
		std::string categoryName = query.getColumn(1);

		CHECK(categoryID == 1);
		CHECK(categoryName == "A");

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		categoryName = query.getColumn(1).getString();

		CHECK(categoryID == 2);
		CHECK(categoryName == "B");

		query.executeStep();

		REQUIRE(!query.hasRow());

		query = SQLite::Statement(database.database(), "SELECT * FROM timeEntryCode");
		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		int codeID = query.getColumn(1);
		std::string codeName = query.getColumn(2);
		int archived = query.getColumn(3);

		CHECK(categoryID == 1);
		CHECK(codeID == 1);
		CHECK(codeName == "Code 1");
		CHECK(archived == 0);

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();
		archived = query.getColumn(3);

		CHECK(categoryID == 1);
		CHECK(codeID == 2);
		CHECK(codeName == "Code 2");
		CHECK(archived == 0);

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();
		archived = query.getColumn(3);

		CHECK(categoryID == 2);
		CHECK(codeID == 3);
		CHECK(codeName == "Code 3");
		CHECK(archived == 0);

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();
		archived = query.getColumn(3);

		CHECK(categoryID == 2);
		CHECK(codeID == 4);
		CHECK(codeName == "Code 4");
		CHECK(archived == 0);

		query.executeStep();

		REQUIRE(!query.hasRow());
	}

	SECTION("Update")
	{
		auto update_category = TimeEntryModifyPacket(RequestID(4));
		update_category.categories.emplace_back(TimeCategoryModType::UPDATE, TimeCategoryID(1), "Test er");
		update_category.codes.emplace_back(TimeCategoryModType::UPDATE, 0, TimeCodeID(1), "Fo o", false);
		update_category.codes.emplace_back(TimeCategoryModType::UPDATE, 0, TimeCodeID(2), "Bar s", false);

		api.process_packet(update_category);

		SQLite::Statement query(database.database(), "SELECT * FROM timeEntryCategory WHERE TimeCategoryID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		int categoryID = query.getColumn(0);
		std::string categoryName = query.getColumn(1);

		CHECK(categoryID == 1);
		CHECK(categoryName == "Test er");

		query.executeStep();

		REQUIRE(!query.hasRow());

		query = SQLite::Statement(database.database(), "SELECT * FROM timeEntryCode WHERE TimeCategoryID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		int codeID = query.getColumn(1);
		std::string codeName = query.getColumn(2);
		int archived = query.getColumn(3);

		CHECK(categoryID == 1);
		CHECK(codeID == 1);
		CHECK(codeName == "Fo o");
		CHECK(archived == 0);

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();
		archived = query.getColumn(3);

		CHECK(categoryID == 1);
		CHECK(codeID == 2);
		CHECK(codeName == "Bar s");
		CHECK(archived == 0);

		query.executeStep();

		REQUIRE(!query.hasRow());
	}

	SECTION("Archive")
	{
		auto update_category = TimeEntryModifyPacket(RequestID(4));
		update_category.categories.emplace_back(TimeCategoryModType::UPDATE, TimeCategoryID(1), "Test er");
		update_category.codes.emplace_back(TimeCategoryModType::UPDATE, 0, TimeCodeID(1), "Fo o", true);
		update_category.codes.emplace_back(TimeCategoryModType::UPDATE, 0, TimeCodeID(2), "Bar s", true);

		api.process_packet(update_category);

		SQLite::Statement query(database.database(), "SELECT * FROM timeEntryCategory WHERE TimeCategoryID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		int categoryID = query.getColumn(0);
		std::string categoryName = query.getColumn(1);

		CHECK(categoryID == 1);
		CHECK(categoryName == "Test er");

		query.executeStep();

		REQUIRE(!query.hasRow());

		query = SQLite::Statement(database.database(), "SELECT * FROM timeEntryCode WHERE TimeCategoryID == 1");
		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		int codeID = query.getColumn(1);
		std::string codeName = query.getColumn(2);
		int archived = query.getColumn(3);

		CHECK(categoryID == 1);
		CHECK(codeID == 1);
		CHECK(codeName == "Fo o");
		CHECK(archived != 0);

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();
		archived = query.getColumn(3);

		CHECK(categoryID == 1);
		CHECK(codeID == 2);
		CHECK(codeName == "Bar s");
		CHECK(archived != 0);

		query.executeStep();

		REQUIRE(!query.hasRow());
	}
}

TEST_CASE("Write Task Time Entry to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	auto addTimeEntry = TimeEntryModifyPacket(RequestID(1));
	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "A");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 1", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 0, TimeCodeID(0), "Code 2", false);

	addTimeEntry.categories.emplace_back(TimeCategoryModType::ADD, TimeCategoryID(0), "B");
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 3", false);
	addTimeEntry.codes.emplace_back(TimeCategoryModType::ADD, 1, TimeCodeID(0), "Code 4", false);

	api.process_packet(addTimeEntry);

	SECTION("Add")
	{
		auto create = CreateTaskMessage(NO_PARENT, RequestID(2), "parent");
		create.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
		create.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

		api.process_packet(create);

		SQLite::Statement query(database.database(), "SELECT * FROM timeEntryTask");
		query.executeStep();

		REQUIRE(query.hasRow());

		int taskID = query.getColumn(0);
		int categoryID = query.getColumn(1);
		int codeID = query.getColumn(2);

		CHECK(taskID == 1);
		CHECK(categoryID == 1);
		CHECK(codeID == 2);

		query.executeStep();

		REQUIRE(query.hasRow());

		taskID = query.getColumn(0);
		categoryID = query.getColumn(1);
		codeID = query.getColumn(2);

		CHECK(taskID == 1);
		CHECK(categoryID == 2);
		CHECK(codeID == 4);

		query.executeStep();

		CHECK(!query.hasRow());
	}

	SECTION("Update - Change Time Codes")
	{
		auto create = CreateTaskMessage(NO_PARENT, RequestID(2), "parent");
		create.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TEST_TIME_CODE_1);
		create.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(4), ""));

		auto update = UpdateTaskMessage(RequestID(2), TaskID(1), NO_PARENT, "parent");
		update.timeEntry.emplace_back(TEST_TIME_CATEGORY_1, TimeCode(TimeCodeID(2), ""));
		update.timeEntry.emplace_back(TEST_TIME_CATEGORY_2, TimeCode(TimeCodeID(3), ""));

		api.process_packet(create);
		api.process_packet(update);

		SQLite::Statement query(database.database(), "SELECT * FROM timeEntryTask");
		query.executeStep();

		REQUIRE(query.hasRow());

		int taskID = query.getColumn(0);
		int categoryID = query.getColumn(1);
		int codeID = query.getColumn(2);

		CHECK(taskID == 1);
		CHECK(categoryID == 1);
		CHECK(codeID == 2);

		query.executeStep();

		REQUIRE(query.hasRow());

		taskID = query.getColumn(0);
		categoryID = query.getColumn(1);
		codeID = query.getColumn(2);

		CHECK(taskID == 1);
		CHECK(categoryID == 2);
		CHECK(codeID == 3);

		query.executeStep();

		CHECK(!query.hasRow());
	}

	// TODO should we add a new time category to all existing tasks?
	// TODO I think I decided to leave category/code data and not delete it? archive instead of delete, so no remove here?
}

TEST_CASE("Write Bugzilla Instance Configurations to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));

	SECTION("Add")
	{
		auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(1);
		configure.groupTasksBy.push_back("product");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		curl.requestResponse.emplace_back("{ \"fields\": [] }");
		curl.requestResponse.emplace_back("{ \"bugs\": [] }");
		curl.requestResponse.emplace_back("{ \"bugs\": [] }");

		api.process_packet(configure);

		SQLite::Statement query(database.database(), "SELECT * FROM bugzilla");
		query.executeStep();

		REQUIRE(query.hasRow());

		int instance = query.getColumn(0);
		std::string name = query.getColumn(1);
		std::string url = query.getColumn(2);
		std::string apiKey = query.getColumn(3);
		std::string userName = query.getColumn(4);
		int rootTask = query.getColumn(5);
		std::int64_t lastRefresh = query.getColumn(6);

		CHECK(instance == 1);
		CHECK(name == "bugzilla");
		CHECK(url == "0.0.0.0");
		CHECK(apiKey == "asfesdFEASfslj");
		CHECK(userName == "test");
		CHECK(rootTask == 1);
		CHECK(lastRefresh == 1737344939870);
	}

	SECTION("Update")
	{
		auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(1);
		configure.groupTasksBy.push_back("product");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		curl.requestResponse.emplace_back("{ \"fields\": [] }");
		curl.requestResponse.emplace_back("{ \"bugs\": [] }");
		curl.requestResponse.emplace_back("{ \"bugs\": [] }");

		api.process_packet(configure);

		configure.instanceID = BugzillaInstanceID(1);
		configure.apiKey = "asdfasdf";

		curl.current = 0;

		api.process_packet(configure);

		SQLite::Statement query(database.database(), "SELECT * FROM bugzilla");
		query.executeStep();

		REQUIRE(query.hasRow());

		int instance = query.getColumn(0);
		std::string name = query.getColumn(1);
		std::string url = query.getColumn(2);
		std::string apiKey = query.getColumn(3);
		std::string userName = query.getColumn(4);
		int rootTask = query.getColumn(5);
		std::int64_t lastRefresh = query.getColumn(6);

		CHECK(instance == 1);
		CHECK(name == "bugzilla");
		CHECK(url == "0.0.0.0");
		CHECK(apiKey == "asdfasdf");
		CHECK(userName == "test");
		CHECK(rootTask == 1);
		CHECK(lastRefresh == 1737345839870);

		query.executeStep();

		CHECK(!query.hasRow());
	}

	SECTION("Refresh")
	{
		auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
		configure.username = "test";
		configure.rootTaskID = TaskID(1);
		configure.groupTasksBy.push_back("product");
		configure.groupTasksBy.push_back("severity");

		configure.labelToField["Priority"] = "priority";
		configure.labelToField["Status"] = "status";

		curl.requestResponse.emplace_back("{ \"fields\": [] }");
		curl.requestResponse.emplace_back("{ \"bugs\": [] }");
		curl.requestResponse.emplace_back("{ \"bugs\": [] }");

		api.process_packet(configure);

		const auto refresh = RequestMessage(PacketType::BUGZILLA_REFRESH, RequestID(3));

		curl.current = 1;

		api.process_packet(refresh);

		SQLite::Statement query(database.database(), "SELECT * FROM bugzilla");
		query.executeStep();

		REQUIRE(query.hasRow());

		int instance = query.getColumn(0);
		std::string name = query.getColumn(1);
		std::string url = query.getColumn(2);
		std::string apiKey = query.getColumn(3);
		std::string userName = query.getColumn(4);
		int rootTask = query.getColumn(5);
		std::int64_t lastRefresh = query.getColumn(6);

		CHECK(instance == 1);
		CHECK(name == "bugzilla");
		CHECK(url == "0.0.0.0");
		CHECK(apiKey == "asfesdFEASfslj");
		CHECK(userName == "test");
		CHECK(rootTask == 1);
		CHECK(lastRefresh == 1737345839870);

		query.executeStep();

		CHECK(!query.hasRow());
	}

	// TODO we haven't implemented this at all yet
	SECTION("Remove")
	{

	}
}

TEST_CASE("Write Bugzilla Group By to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));

	auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(1);
	configure.groupTasksBy.push_back("product");
	configure.groupTasksBy.push_back("severity");

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	curl.requestResponse.emplace_back("{ \"fields\": [] }");
	curl.requestResponse.emplace_back("{ \"bugs\": [] }");
	curl.requestResponse.emplace_back("{ \"bugs\": [] }");

	api.process_packet(configure);

	SQLite::Statement query(database.database(), "SELECT * FROM bugzillaGroupBy");
	query.executeStep();

	REQUIRE(query.hasRow());

	int instanceID = query.getColumn(0);
	std::string groupBy = query.getColumn(1);

	CHECK(instanceID == 1);
	CHECK(groupBy == "product,severity");
}

TEST_CASE("Write Bugzilla Bug ID to Task ID to Database", "[database]")
{
	TestClock clock;
	curlTest curl;
	TestPacketSender sender;
	DatabaseImpl database(":memory:", sender);

	API api = API(clock, curl, database, sender);

	std::vector<std::unique_ptr<Message>> output;

	api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"));

	auto configure = BugzillaInfoMessage(BugzillaInstanceID(0), "bugzilla", "0.0.0.0", "asfesdFEASfslj");
	configure.username = "test";
	configure.rootTaskID = TaskID(1);
	configure.groupTasksBy.push_back("priority");
	configure.groupTasksBy.push_back("severity");

	configure.labelToField["Priority"] = "priority";
	configure.labelToField["Status"] = "status";

	const std::string fieldsResponse = R"bugs_data(
		{
		  "fields": [
			{
			  "display_name": "Priority",
			  "name": "priority",
			  "type": 2,
			  "is_mandatory": false,
			  "value_field": null,
			  "values": [
				{
				  "sortkey": 100,
				  "sort_key": 100,
				  "visibility_values": [],
				  "name": "P1"
				},
				{
				  "sort_key": 200,
				  "name": "P2",
				  "visibility_values": [],
				  "sortkey": 200
				},
				{
				  "sort_key": 300,
				  "visibility_values": [],
				  "name": "P3",
				  "sortkey": 300
				},
				{
				  "sort_key": 400,
				  "name": "P4",
				  "visibility_values": [],
				  "sortkey": 400
				}
			  ],
			  "visibility_values": [],
			  "visibility_field": null,
			  "is_on_bug_entry": false,
			  "is_custom": false,
			  "id": 13
			},
			{
			  "display_name": "Severity",
			  "name": "severity",
			  "type": 2,
			  "is_mandatory": false,
			  "value_field": null,
			  "values": [
				{
				  "sortkey": 100,
				  "sort_key": 100,
				  "visibility_values": [],
				  "name": "Nitpick"
				},
				{
				  "sort_key": 200,
				  "name": "Minor",
				  "visibility_values": [],
				  "sortkey": 200
				},
				{
				  "sort_key": 300,
				  "visibility_values": [],
				  "name": "Critical",
				  "sortkey": 300
				},
				{
				  "sort_key": 400,
				  "name": "Blocker",
				  "visibility_values": [],
				  "sortkey": 400
				}
			  ],
			  "visibility_values": [],
			  "visibility_field": null,
			  "is_on_bug_entry": false,
			  "is_custom": false,
			  "id": 14
			}
		  ]
		}
	)bugs_data";

	curl.requestResponse.emplace_back(fieldsResponse);
	curl.requestResponse.emplace_back("{ \"bugs\": [ { \"id\": 50, \"assigned_to\": \"test\", \"summary\": \"bug 1\", \"status\": \"Assigned\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
		"{ \"id\": 55, \"assigned_to\": \"test\", \"summary\": \"bug 2\", \"status\": \"Changes Made\", \"priority\": \"P2\", \"severity\": \"Minor\" },"
		"{ \"id\": 60, \"assigned_to\": \"test\", \"summary\": \"bug 3\", \"status\": \"Changes Made\", \"priority\": \"P1\", \"severity\": \"Critical\" },"
		"{ \"id\": 65, \"assigned_to\": \"test\", \"summary\": \"bug 4\", \"status\": \"Reviewed\", \"priority\": \"P3\", \"severity\": \"Blocker\" },"
		"{ \"id\": 70, \"assigned_to\": \"test\", \"summary\": \"bug 5\", \"status\": \"Confirmed\", \"priority\": \"P4\", \"severity\": \"Nitpick\" } ] }");
	curl.requestResponse.push_back(curl.requestResponse.back());

	api.process_packet(configure);

	SQLite::Statement query(database.database(), "SELECT * FROM bugzillaBugToTask");
	query.executeStep();

	REQUIRE(query.hasRow());

	int instanceID = query.getColumn(0);
	int bug = query.getColumn(1);
	int task = query.getColumn(2);

	CHECK(instanceID == 1);
	CHECK(bug == 50);
	CHECK(task == 4);

	query.executeStep();
	REQUIRE(query.hasRow());

	instanceID = query.getColumn(0);
	bug = query.getColumn(1);
	task = query.getColumn(2);

	CHECK(instanceID == 1);
	CHECK(bug == 55);
	CHECK(task == 5);

	query.executeStep();
	REQUIRE(query.hasRow());

	instanceID = query.getColumn(0);
	bug = query.getColumn(1);
	task = query.getColumn(2);

	CHECK(instanceID == 1);
	CHECK(bug == 60);
	CHECK(task == 8);

	query.executeStep();
	REQUIRE(query.hasRow());

	instanceID = query.getColumn(0);
	bug = query.getColumn(1);
	task = query.getColumn(2);

	CHECK(instanceID == 1);
	CHECK(bug == 65);
	CHECK(task == 11);

	query.executeStep();
	REQUIRE(query.hasRow());

	instanceID = query.getColumn(0);
	bug = query.getColumn(1);
	task = query.getColumn(2);

	CHECK(instanceID == 1);
	CHECK(bug == 70);
	CHECK(task == 14);

	query.executeStep();
	CHECK(!query.hasRow());
}
