#include "database.hpp"
#include "api.hpp"

DatabaseImpl::DatabaseImpl(const std::string& file)
	: m_database(file, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
	try
	{
		m_database.exec("create table if not exists tasks (TaskID integer PRIMARY KEY, Name text, ParentID integer, State int, CreateTime bigint, FinishTime bigint)");
		m_database.exec("create table if not exists timeEntryCategory (TimeCategoryID integer PRIMARY KEY, TimeCategoryName text)");
		m_database.exec("create table if not exists timeEntryCode (TimeCategoryID integer, TimeCodeID integer, TimeCodeName text, PRIMARY KEY (TimeCategoryID, TimeCodeID))");
		m_database.exec("create table if not exists timeEntryTask (TaskID integer, TimeCategoryID integer, TimeCodeID integer, PRIMARY KEY (TaskID, TimeCategoryID))");
		m_database.exec("create table if not exists timeEntrySession (TaskID integer, SessionIndex integer, TimeCategoryID integer, TimeCodeID integer, StartTime bigint, StopTime bigint, PRIMARY KEY (TaskID, SessionIndex, TimeCategoryID))");
		m_database.exec("create table if not exists bugzilla (BugzillaInstanceID integer PRIMARY KEY, Name text, URL text, APIKey text, UserName text, RootTaskID integer, LastRefresh bigint)");
		m_database.exec("create table if not exists bugzillaGroupBy (BugzillaInstanceID integer PRIMARY KEY, Field text)");
		m_database.exec("create table if not exists bugzillaBugToTask (BugzillaInstanceID integer, BugID integer, TaskID integer, PRIMARY KEY (BugzillaInstanceID, BugID))");
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

	write_task_time_entry(task);
	write_sessions(task);
}

void DatabaseImpl::write_bugzilla_instance(const BugzillaInstance& instance)
{
	SQLite::Statement insert(m_database, "insert or replace into bugzilla values(?, ?, ?, ?, ?, ?, ?)");
	insert.bind(1, instance.instanceID._val);
	insert.bind(2, instance.bugzillaName);
	insert.bind(3, instance.bugzillaURL);
	insert.bind(4, instance.bugzillaApiKey);
	insert.bind(5, instance.bugzillaUsername);
	insert.bind(6, instance.bugzillaRootTaskID._val);
	insert.bind(7, instance.lastBugzillaRefresh.value_or(std::chrono::milliseconds(0)).count());

	try
	{
		insert.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	write_bugzilla_group_by(instance);
	write_bugzilla_bug_to_task(instance);
}

void DatabaseImpl::write_task_time_entry(const Task& task)
{
	for (const TimeEntry& entry : task.timeEntry)
	{
		SQLite::Statement insert(m_database, "insert or replace into timeEntryTask values(?, ?, ?)");
		insert.bind(1, task.taskID()._val);
		insert.bind(2, entry.categoryID._val);
		insert.bind(3, entry.codeID._val);

		try
		{
			insert.exec();
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
}

void DatabaseImpl::write_sessions(const Task& task)
{
	std::int32_t index = 0;

	for (const TaskTimes& times : task.m_times)
	{
		// TODO we don't write anything when we don't have time config data
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

void DatabaseImpl::write_time_entry_config(const TimeCategory& entry)
{
	SQLite::Statement insert_cat(m_database, "insert or replace into timeEntryCategory values (?, ?)");
	insert_cat.bind(1, entry.id._val);
	insert_cat.bind(2, entry.name);

	try
	{
		insert_cat.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	for (const TimeCode& code : entry.codes)
	{
		SQLite::Statement insert(m_database, "insert or replace into timeEntryCode values (?, ?, ?)");

		insert.bind(1, entry.id._val);
		insert.bind(2, code.id._val);
		insert.bind(3, code.name);

		try
		{
			insert.exec();
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
}

void DatabaseImpl::remove_time_category(const TimeCategory& entry)
{
	SQLite::Statement remove_cat(m_database, "delete from timeEntryCategory where TimeCategoryID == ?");
	remove_cat.bind(1, entry.id._val);

	try
	{
		remove_cat.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	SQLite::Statement remove_code(m_database, "delete from timeEntryCode where TimeCategoryID == ?");
	remove_code.bind(1, entry.id._val);

	try
	{
		remove_code.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void DatabaseImpl::remove_time_code(const TimeCategory& entry, const TimeCode& code)
{
	SQLite::Statement remove_code(m_database, "delete from timeEntryCode where TimeCodeID == ?");
	remove_code.bind(1, code.id._val);

	try
	{
		remove_code.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void DatabaseImpl::write_bugzilla_group_by(const BugzillaInstance& instance)
{
	SQLite::Statement insert(m_database, "insert or replace into bugzillaGroupBy values(?, ?)");

	std::string group_by_full;
	bool first = true;

	for (const std::string& group_by : instance.bugzillaGroupTasksBy)
	{
		if (!first)
		{
			group_by_full += ',';
		}
		first = false;

		group_by_full += group_by;
	}

	insert.bind(1, instance.instanceID._val);
	insert.bind(2, group_by_full);

	try
	{
		insert.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void DatabaseImpl::write_bugzilla_bug_to_task(const BugzillaInstance& instance)
{
	for (auto&& [bug, task] : instance.bugToTaskID)
	{
		SQLite::Statement insert(m_database, "insert or replace into bugzillaBugToTask values(?, ?, ?)");
		insert.bind(1, instance.instanceID._val);
		insert.bind(2, bug);
		insert.bind(3, task._val);

		try
		{
			insert.exec();
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
		}
	}
}
