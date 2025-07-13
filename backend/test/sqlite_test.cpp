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
	has_table("timeConfig");
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

	api.process_packet(CreateTaskMessage(NO_PARENT, RequestID(1), "this is a test"));

	auto task = Task("this is a test", TaskID(1), NO_PARENT, std::chrono::milliseconds(1737344039870));

	REQUIRE(helper.database.tasks_written.size() == 1);
	CHECK(task == helper.database.tasks_written[0]);
	SQLite::Statement query(database.database(), "SELECT * FROM tasks WHERE TaskID == 1");
	query.exec();

	REQUIRE(query.hasRow());
}

TEST_CASE("Write Task Session to Database", "[database]")
{

}

TEST_CASE("Write Task Time Entry to Database", "[database]")
{

}