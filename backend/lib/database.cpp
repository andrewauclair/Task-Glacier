#include "database.hpp"
#include "api.hpp"

#include <format>

/*
* Database Versions for Reference
* 
* 1 = 0.3.3
* 2 = 0.4.0
*/
static constexpr std::int32_t CURRENT_DATABASE_VERSION = 2;

DatabaseImpl::DatabaseImpl(const std::string& file)
	: m_database(file, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
	try
	{
		m_database.exec("create table if not exists tasks (TaskID integer PRIMARY KEY, Name text, ParentID integer, State integer, CreateTime bigint, FinishTime bigint, Locked integer, ServerControlled integer, IndexInParent integer)");
		m_database.exec("create table if not exists timeEntryCategory (TimeCategoryID integer PRIMARY KEY, TimeCategoryName text)");
		m_database.exec("create table if not exists timeEntryCode (TimeCategoryID integer, TimeCodeID integer, TimeCodeName text, PRIMARY KEY (TimeCategoryID, TimeCodeID))");
		m_database.exec("create table if not exists timeEntryTask (TaskID integer, TimeCategoryID integer, TimeCodeID integer, PRIMARY KEY (TaskID, TimeCategoryID))");
		m_database.exec("create table if not exists timeEntrySession (TaskID integer, SessionIndex integer, TimeCategoryID integer, TimeCodeID integer, StartTime bigint, StopTime bigint, PRIMARY KEY (TaskID, SessionIndex, TimeCategoryID))");
		m_database.exec("create table if not exists bugzilla (BugzillaInstanceID integer PRIMARY KEY, Name text, URL text, APIKey text, UserName text, RootTaskID integer, LastRefresh bigint)");
		m_database.exec("create table if not exists bugzillaGroupBy (BugzillaInstanceID integer PRIMARY KEY, Field text)");
		m_database.exec("create table if not exists bugzillaBugToTask (BugzillaInstanceID integer, BugID integer, TaskID integer, PRIMARY KEY (BugzillaInstanceID, BugID))");
		m_database.exec("create table if not exists nextIDs (Name text PRIMARY KEY, ID integer)");

		SQLite::Statement get_version(m_database, "PRAGMA user_version;");
		get_version.executeStep();

		int version = get_version.getColumn(0);

		switch (version)
		{
		case 1: // 1 - 0.3.3
		{
			// introduced a IndexInParent column to the tasks table. the values will be corrected when reordering in the UI
			m_database.exec("alter table tasks add column IndexInParent integer");

			std::map<int, int> nextIndex;

			SQLite::Statement query(m_database, "select * from tasks");
			query.executeStep();

			while (query.hasRow())
			{
				int taskID = query.getColumn(0);
				int parentID = query.getColumn(2);

				m_database.exec(std::format("update tasks set IndexInParent = {} where TaskID = {}", nextIndex[parentID], taskID));
				++nextIndex[parentID];

				query.executeStep();
			}
			break;
		}
		}
		SQLite::Statement set_version(m_database, std::format("PRAGMA user_version={};", CURRENT_DATABASE_VERSION));
		set_version.executeStep();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void DatabaseImpl::load(Bugzilla& bugzilla, MicroTask& app, API& api)
{
	load_time_entry(app);
	load_tasks(app);
	load_bugzilla_instances(bugzilla, app);
	load_next_ids(bugzilla, app);
}

void DatabaseImpl::write_task(const Task& task)
{
	SQLite::Statement insert(m_database, "insert or replace into tasks values(?, ?, ?, ?, ?, ?, ?, ?, ?)");
	insert.bind(1, task.taskID()._val);
	insert.bind(2, task.m_name);
	insert.bind(3, task.parentID()._val);
	insert.bind(4, static_cast<int>(task.state));
	insert.bind(5, task.createTime().count());
	insert.bind(6, task.m_finishTime.value_or(std::chrono::milliseconds(0)).count());
	insert.bind(7, task.locked);
	insert.bind(8, task.serverControlled);
	insert.bind(9, task.indexInParent);

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

void DatabaseImpl::write_next_task_id(TaskID nextID)
{
	SQLite::Statement insert(m_database, "insert or replace into nextIDs values ('task', ?)");
	insert.bind(1, nextID._val);

	try
	{
		insert.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
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

void DatabaseImpl::write_next_bugzilla_instance_id(BugzillaInstanceID nextID)
{
	SQLite::Statement insert(m_database, "insert or replace into nextIDs values ('bugzilla', ?)");
	insert.bind(1, nextID._val);

	try
	{
		insert.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
void DatabaseImpl::load_time_entry(MicroTask& app)
{
	SQLite::Statement query_category(m_database, "SELECT * FROM timeEntryCategory");
	SQLite::Statement query_code(m_database, "SELECT * FROM timeEntryCode");

	std::map<TimeCategoryID, TimeCategory> categories;

	query_category.executeStep();

	while (query_category.hasRow())
	{
		int id = query_category.getColumn(0);
		auto cat = TimeCategory{ TimeCategoryID(id) };
		cat.name = query_category.getColumn(1).getString();

		categories.emplace(cat.id, cat);

		query_category.executeStep();
	}

	query_code.executeStep();

	while (query_code.hasRow())
	{
		int catID = query_code.getColumn(0);
		int codeID = query_code.getColumn(1);
		std::string code_name = query_code.getColumn(2);

		categories.at(TimeCategoryID(catID)).codes.emplace_back(TimeCodeID(codeID), code_name);

		query_code.executeStep();
	}

	std::vector<TimeCategory> timeCategories;

	for (auto&& [id, cat] : categories)
	{
		timeCategories.push_back(cat);
	}

	app.load_time_entry(timeCategories);
}

void DatabaseImpl::load_tasks(MicroTask& app)
{
	SQLite::Statement query(m_database, "SELECT * FROM tasks");
	query.executeStep();

	while (query.hasRow())
	{
		int taskID = query.getColumn(0);
		std::string name = query.getColumn(1);
		int parentID = query.getColumn(2);
		int state = query.getColumn(3);
		std::int64_t create_time = query.getColumn(4);
		std::int64_t finish_time = query.getColumn(5);
		int locked = query.getColumn(6);
		int serverControlled = query.getColumn(7);
		int indexInParent = query.getColumn(8);

		Task task = Task(std::move(name), TaskID(taskID), TaskID(parentID), std::chrono::milliseconds(create_time));
		task.state = static_cast<TaskState>(state);
		task.m_finishTime = finish_time == 0 ? std::nullopt : std::optional(std::chrono::milliseconds(finish_time));
		task.indexInParent = indexInParent;
		task.locked = locked;
		task.serverControlled = serverControlled;

		SQLite::Statement query_time_entry(m_database, "SELECT * FROM timeEntryTask WHERE TaskID == ?");
		query_time_entry.bind(1, taskID);
		query_time_entry.executeStep();

		while (query_time_entry.hasRow())
		{
			int catID = query_time_entry.getColumn(1);
			int codeID = query_time_entry.getColumn(2);

			task.timeEntry.emplace_back(TimeCategoryID(catID), TimeCodeID(codeID));

			query_time_entry.executeStep();
		}

		SQLite::Statement query_sessions(m_database, "SELECT * FROM timeEntrySession WHERE TaskID == ?; ORDER BY Index ASC;");
		query_sessions.bind(1, taskID);
		query_sessions.executeStep();

		while (query_sessions.hasRow())
		{
			int index = query_sessions.getColumn(1);
			int catID = query_sessions.getColumn(2);
			int codeID = query_sessions.getColumn(3);
			std::int64_t start_time = query_sessions.getColumn(4);
			std::int64_t stop_time = query_sessions.getColumn(5);
			
			if (task.m_times.size() > index)
			{
				task.m_times.back().timeEntry.emplace_back(TimeCategoryID(catID), TimeCodeID(codeID));
			}
			else
			{
				TaskTimes times{ std::chrono::milliseconds(start_time) };
				times.stop = stop_time == 0 ? std::nullopt : std::optional(std::chrono::milliseconds(stop_time));
				times.timeEntry.emplace_back(TimeCategoryID(catID), TimeCodeID(codeID));

				task.m_times.push_back(times);
			}

			query_sessions.executeStep();
		}

		app.load_task(task);

		query.executeStep();
	}
}

void DatabaseImpl::load_bugzilla_instances(Bugzilla& bugzilla, MicroTask& app)
{
	SQLite::Statement query_instances(m_database, "SELECT * FROM bugzilla");
	query_instances.executeStep();

	while (query_instances.hasRow())
	{
		int instanceID = query_instances.getColumn(0);
		std::string name = query_instances.getColumn(1);
		std::string url = query_instances.getColumn(2);
		std::string apiKey = query_instances.getColumn(3);
		std::string userName = query_instances.getColumn(4);
		int rootTaskID = query_instances.getColumn(5);
		std::int64_t last_refresh = query_instances.getColumn(6);

		query_instances.executeStep();

		BugzillaInstance instance = BugzillaInstance(BugzillaInstanceID(instanceID));
		instance.bugzillaName = name;
		instance.bugzillaURL = url;
		instance.bugzillaApiKey = apiKey;
		instance.bugzillaUsername = userName;
		instance.bugzillaRootTaskID = TaskID(rootTaskID);
		instance.lastBugzillaRefresh = std::chrono::milliseconds(last_refresh);

		SQLite::Statement query_group_by(m_database, "SELECT * FROM bugzillaGroupBy WHERE BugzillaInstanceID == ?");
		query_group_by.bind(1, instanceID);

		query_group_by.executeStep();

		const auto groupBy = split(query_group_by.getColumn(1).getString(), ',');

		for (const std::string& g : groupBy)
		{
			instance.bugzillaGroupTasksBy.push_back(g);
		}

		SQLite::Statement query_bug_to_task(m_database, "SELECT * FROM bugzillaBugToTask WHERE BugzillaInstanceID == ?");
		query_bug_to_task.bind(1, instanceID);
		query_bug_to_task.executeStep();

		while (query_bug_to_task.hasRow())
		{
			int bug = query_bug_to_task.getColumn(1);
			int task = query_bug_to_task.getColumn(2);

			instance.bugToTaskID.emplace(bug, TaskID(task));

			query_bug_to_task.executeStep();
		}

		bugzilla.load_instance(instance);
	}
}

void DatabaseImpl::load_next_ids(Bugzilla& bugzilla, MicroTask& app)
{
	SQLite::Statement query(m_database, "SELECT * FROM nextIDs");
	query.executeStep();

	while (query.hasRow())
	{
		if (query.getColumn(0).getString() == "bugzilla")
		{
			bugzilla.next_instance_id(BugzillaInstanceID(query.getColumn(1).getInt()));
		}
		else if (query.getColumn(0).getString() == "task")
		{
			app.m_nextTaskID = TaskID(query.getColumn(1).getInt());
		}
		else if (query.getColumn(0).getString() == "category")
		{
			app.m_nextTimeCategoryID = TimeCategoryID(query.getColumn(1).getInt());
		}
		else if (query.getColumn(0).getString() == "code")
		{
			app.m_nextTimeCodeID = TimeCodeID(query.getColumn(1).getInt());
		}

		query.executeStep();
	}
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
		if (times.timeEntry.empty())
		{
			SQLite::Statement insert(m_database, "insert or replace into timeEntrySession values(?, ?, ?, ?, ?, ?)");
			insert.bind(1, task.taskID()._val);
			insert.bind(2, index);
			insert.bind(3, 0);
			insert.bind(4, 0);
			insert.bind(5, times.start.count());
			insert.bind(6, times.stop.value_or(std::chrono::milliseconds(0)).count());

			try
			{
				insert.exec();
			}
			catch (const std::exception& e)
			{
				std::cerr << e.what() << std::endl;
			}
		}

		for (const TimeEntry& entry : times.timeEntry)
		{
			SQLite::Statement insert(m_database, "insert or replace into timeEntrySession values(?, ?, ?, ?, ?, ?)");
			insert.bind(1, task.taskID()._val);
			insert.bind(2, index);
			insert.bind(3, entry.categoryID._val);
			insert.bind(4, entry.codeID._val);
			insert.bind(5, times.start.count());
			insert.bind(6, times.stop.value_or(std::chrono::milliseconds(0)).count());
			
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

void DatabaseImpl::write_next_time_category_id(TimeCategoryID nextID)
{
	SQLite::Statement insert(m_database, "insert or replace into nextIDs values ('category', ?)");
	insert.bind(1, nextID._val);

	try
	{
		insert.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void DatabaseImpl::write_next_time_code_id(TimeCodeID nextID)
{
	SQLite::Statement insert(m_database, "insert or replace into nextIDs values ('code', ?)");
	insert.bind(1, nextID._val);

	try
	{
		insert.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
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

void DatabaseImpl::start_transaction()
{
	SQLite::Statement start(m_database, "BEGIN TRANSACTION;");

	try
	{
		start.exec();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}
}

void DatabaseImpl::finish_transaction()
{
	SQLite::Statement start(m_database, "COMMIT;");

	try
	{
		start.exec();
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
