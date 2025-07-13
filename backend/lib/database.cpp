#include "database.hpp"
#include "api.hpp"

DatabaseImpl::DatabaseImpl(const std::string& file)
	: m_database(file, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
	try
	{
		m_database.exec("create table if not exists tasks (TaskID integer PRIMARY KEY, Name text, ParentID integer, State int, CreateTime bigint, FinishTime bigint)");
		m_database.exec("create table if not exists timeConfig (TimeCategoryID integer, TimeCodeID integer)");
		m_database.exec("create table if not exists timeEntryTask (TaskID integer, TimeCategoryID integer, TimeCodeID integer)");
		m_database.exec("create table if not exists timeEntrySession (TaskID integer, SessionIndex integer, TimeCategoryID integer, TimeCodeID integer, StartTime bigint, StopTime bigint, PRIMARY KEY (TaskID, SessionIndex, TimeCategoryID))");
		m_database.exec("create table if not exists bugzilla (BugzillaConfigID integer PRIMARY KEY, Name text, URL text, APIKey text, UserName text, RootTaskID integer, LastRefresh integer)");
		m_database.exec("create table if not exists bugzillaGroupBy (BugzillaConfigID integer, Field text)");
		m_database.exec("create table if not exists bugzillaBugToTask (BugzillaConfigID integer, BugID integer, TaskID integer)");
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void DatabaseImpl::write_task(const Task& task)
{
	SQLite::Statement insert(m_database, "insert or replace into tasks values(?, ?, ?, ?, ?, ?)");
	insert.bind(1, task.taskID()._val);
	insert.bind(2, task.m_name);
	insert.bind(3, task.parentID()._val);
	insert.bind(4, static_cast<int>(task.state));
	insert.bind(5, task.createTime().count());
	insert.bind(6, task.m_finishTime.value_or(std::chrono::milliseconds(0)).count());

	try
	{
		insert.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	write_sessions(task);
}

void DatabaseImpl::write_sessions(const Task& task)
{
	std::int32_t index = 0;

	for (const TaskTimes& times : task.m_times)
	{
		for (const TimeEntry& entry : times.timeEntry)
		{
			SQLite::Statement insert(m_database, "insert or replace into timeEntrySession values(?, ?, ?, ?, ?, ?)");
			insert.bind(1, task.taskID()._val);
			insert.bind(2, index);
			insert.bind(3, entry.categoryID._val);
			insert.bind(4, times.start.count());
			insert.bind(5, times.stop.value_or(std::chrono::milliseconds(0)).count());
			insert.bind(6, entry.codeID._val);

			try
			{
				insert.exec();
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}

		index++;
	}
}
