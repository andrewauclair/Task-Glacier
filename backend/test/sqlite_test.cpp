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

	// check that we have the proper tables
	SQLite::Statement query(db.database(), "SELECT name FROM sqlite_schema WHERE type='table' AND name='tasks'");
	query.executeStep();
	
	CHECK(query.hasRow());
}