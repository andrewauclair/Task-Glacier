#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>

#include "packets.hpp"
#include "database.hpp"
#include "utils.h"

#include <filesystem>

#include "SQLiteCpp/Database.h"

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

TEST_CASE("Create Database", "[database]")
{
	DatabaseImpl db(":memory:");

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

TEST_CASE("Write Task to Database", "[database]")
{
	TestClock clock;
	curlTest curl;

	DatabaseImpl database(":memory:");

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, curl, fileInput, fileOutput, database);

	std::vector<std::unique_ptr<Message>> output;

	api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "parent"), output);
	api.process_packet(CreateTaskMessage(TaskID(1), RequestID(2), "this is a test"), output);
	api.process_packet(TaskMessage(PacketType::FINISH_TASK, RequestID(3), TaskID(2)), output);

	auto task = Task("this is a test", TaskID(1), NO_PARENT, std::chrono::milliseconds(1737344039870));

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
	CHECK(state == 2);
	CHECK(create_time == 1737345839870);
	CHECK(finish_time == 1737347639870);
}

TEST_CASE("Write Task Session to Database", "[database]")
{
	TestClock clock;
	curlTest curl;

	DatabaseImpl database(":memory:");

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, curl, fileInput, fileOutput, database);

	std::vector<std::unique_ptr<Message>> output;

	auto modify = TimeEntryModifyPacket(RequestID(1), TimeCategoryModType::ADD, {});
	auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A", "A");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

	auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B", "B");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");

	api.process_packet(modify, output);

	auto create = CreateTaskMessage(NO_PARENT, RequestID(2), "parent");
	create.timeEntry.emplace_back(TimeCategoryID(1), TimeCodeID(1));
	create.timeEntry.emplace_back(TimeCategoryID(2), TimeCodeID(4));

	api.process_packet(create, output);
	api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(3), TaskID(1)), output);
	api.process_packet(TaskMessage(PacketType::STOP_TASK, RequestID(4), TaskID(1)), output);
	api.process_packet(TaskMessage(PacketType::START_TASK, RequestID(5), TaskID(1)), output);

	SQLite::Statement query(database.database(), "SELECT * FROM timeEntrySession WHERE TaskID == 1");
	query.executeStep();

	REQUIRE(query.hasRow());

	int id = query.getColumn(0);
	int sessionIndex = query.getColumn(1);
	int timeCategoryID = query.getColumn(2);
	std::int64_t start = query.getColumn(3);
	std::int64_t stop = query.getColumn(4);
	int timeCodeID = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(start == 1737345839870);
	CHECK(stop == 1737347639870);
	CHECK(timeCategoryID == 1);
	CHECK(timeCodeID == 1);

	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	start = query.getColumn(3);
	stop = query.getColumn(4);
	timeCodeID = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 0);
	CHECK(start == 1737345839870);
	CHECK(stop == 1737347639870);
	CHECK(timeCategoryID == 2);
	CHECK(timeCodeID == 4);

	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	start = query.getColumn(3);
	stop = query.getColumn(4);
	timeCodeID = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 1);
	CHECK(start == 1737349439870);
	CHECK(stop == 0);
	CHECK(timeCategoryID == 1);
	CHECK(timeCodeID == 1);

	query.executeStep();

	REQUIRE(query.hasRow());

	id = query.getColumn(0);
	sessionIndex = query.getColumn(1);
	timeCategoryID = query.getColumn(2);
	start = query.getColumn(3);
	stop = query.getColumn(4);
	timeCodeID = query.getColumn(5);

	CHECK(id == 1);
	CHECK(sessionIndex == 1);
	CHECK(start == 1737349439870);
	CHECK(stop == 0);
	CHECK(timeCategoryID == 2);
	CHECK(timeCodeID == 4);

	query.executeStep();

	REQUIRE(!query.hasRow());
}

TEST_CASE("Write Time Configuration to Database", "[database]")
{
	TestClock clock;
	curlTest curl;

	DatabaseImpl database(":memory:");

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, curl, fileInput, fileOutput, database);

	std::vector<std::unique_ptr<Message>> output;

	auto modify = TimeEntryModifyPacket(RequestID(1), TimeCategoryModType::ADD, {});
	auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A", "A");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

	auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B", "B");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");

	api.process_packet(modify, output);

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

		CHECK(categoryID == 1);
		CHECK(codeID == 1);
		CHECK(codeName == "Code 1");

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();

		CHECK(categoryID == 1);
		CHECK(codeID == 2);
		CHECK(codeName == "Code 2");

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();

		CHECK(categoryID == 2);
		CHECK(codeID == 3);
		CHECK(codeName == "Code 3");

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();

		CHECK(categoryID == 2);
		CHECK(codeID == 4);
		CHECK(codeName == "Code 4");

		query.executeStep();

		REQUIRE(!query.hasRow());
	}

	SECTION("Update")
	{
		auto cat = TimeCategory(TimeCategoryID(1));
		cat.name = "Test er";
		cat.label = "TST R";
		cat.codes.clear();
		cat.codes.push_back(TimeCode{ TimeCodeID(1), "Fo o" });
		cat.codes.push_back(TimeCode{ TimeCodeID(2), "Bar s" });

		auto update_category = TimeEntryModifyPacket(RequestID(4), TimeCategoryModType::UPDATE, {});
		update_category.timeCategories.push_back(cat);

		api.process_packet(update_category, output);

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

		CHECK(categoryID == 1);
		CHECK(codeID == 1);
		CHECK(codeName == "Fo o");

		query.executeStep();

		REQUIRE(query.hasRow());

		categoryID = query.getColumn(0);
		codeID = query.getColumn(1);
		codeName = query.getColumn(2).getString();

		CHECK(categoryID == 1);
		CHECK(codeID == 2);
		CHECK(codeName == "Bar s");

		query.executeStep();

		REQUIRE(!query.hasRow());
	}

	SECTION("Remove")
	{
		SECTION("Category")
		{

		}

		SECTION("Code")
		{

		}
	}
}

TEST_CASE("Write Task Time Entry to Database", "[database]")
{
	TestClock clock;
	curlTest curl;

	DatabaseImpl database(":memory:");

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, curl, fileInput, fileOutput, database);

	std::vector<std::unique_ptr<Message>> output;

	auto modify = TimeEntryModifyPacket(RequestID(1), TimeCategoryModType::ADD, {});
	auto& newCategory1 = modify.timeCategories.emplace_back(TimeCategoryID(0), "A", "A");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 1");
	newCategory1.codes.emplace_back(TimeCodeID(0), "Code 2");

	auto& newCategory2 = modify.timeCategories.emplace_back(TimeCategoryID(0), "B", "B");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 3");
	newCategory2.codes.emplace_back(TimeCodeID(0), "Code 4");

	api.process_packet(modify, output);

	auto create = CreateTaskMessage(NO_PARENT, RequestID(2), "parent");
	create.timeEntry.emplace_back(TimeCategoryID(1), TimeCodeID(1));
	create.timeEntry.emplace_back(TimeCategoryID(2), TimeCodeID(4));

	api.process_packet(create, output);
}

TEST_CASE("Write Bugzilla Instance Configurations to Database", "[database]")
{
	TestClock clock;
	curlTest curl;

	DatabaseImpl database(":memory:");

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, curl, fileInput, fileOutput, database);

	std::vector<std::unique_ptr<Message>> output;

	SECTION("Remove")
	{

	}
}

TEST_CASE("Write Bugzilla Group By to Database", "[database]")
{
	TestClock clock;
	curlTest curl;

	DatabaseImpl database(":memory:");

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, curl, fileInput, fileOutput, database);

	std::vector<std::unique_ptr<Message>> output;
}

TEST_CASE("Write Bugzilla Bug ID to Task ID to Database", "[database]")
{
	TestClock clock;
	curlTest curl;

	DatabaseImpl database(":memory:");

	std::istringstream fileInput;
	std::ostringstream fileOutput;

	API api = API(clock, curl, fileInput, fileOutput, database);

	std::vector<std::unique_ptr<Message>> output;
}
