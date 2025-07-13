#pragma once

#include "packets.hpp"

#include <SQLiteCpp/Database.h>

class Task;
struct BugzillaInstance;
class Bugzilla;
class MicroTask;
class API;
struct TaskTimes;
struct TimeCategory;

struct Database
{
	virtual void load(Bugzilla& bugzilla, MicroTask& app, API& api) = 0;

	// write task
	virtual void write_task(const Task& task) = 0;
	virtual void next_task_id(TaskID taskID) = 0;

	// write bugzilla config
	virtual void write_bugzilla_instance(const BugzillaInstance& instance) = 0;
	virtual void remove_bugzilla_instance(int ID) = 0;
	virtual void next_bugzilla_instance_id(int ID) = 0;
	virtual void bugzilla_refreshed(int ID) = 0;

	// write time entry configuration
	// write sessions
	virtual void write_session(TaskID task, const TaskTimes& session) = 0;
	virtual void remove_session() = 0;

	// write time entries
	virtual void write_time_entry(TaskID task) = 0;
	virtual void remove_time_entry() = 0;

	virtual void write_time_entry_config(const TimeCategory& entry) = 0;
	virtual void remove_time_category(const TimeCategory& entry) = 0;
	virtual void remove_time_code(const TimeCategory& entry, const TimeCode& code) = 0;
};

struct DatabaseImpl : Database
{
	DatabaseImpl(const std::string& file);

	SQLite::Database& database() { return m_database; }

	void load(Bugzilla& bugzilla, MicroTask& app, API& api) override {}

	// write task
	void write_task(const Task& task) override;
	void next_task_id(TaskID taskID) override {}

	// write bugzilla config
	void write_bugzilla_instance(const BugzillaInstance& instance) override;
	void remove_bugzilla_instance(int ID) override {}
	void next_bugzilla_instance_id(int ID) override {}
	void bugzilla_refreshed(int ID) override {}

	// write time entry configuration
	// write sessions
	void write_session(TaskID task, const TaskTimes& session) override {}
	void remove_session() override {}

	// write time entries
	void write_time_entry(TaskID task) override {}
	void remove_time_entry() override {}

	void write_time_entry_config(const TimeCategory& entry) override;
	void remove_time_category(const TimeCategory& entry) override;
	void remove_time_code(const TimeCategory& entry, const TimeCode& code) override;

private:
	void load_tasks(Bugzilla& bugzilla, MicroTask& app, API& api);
	void load_bugzilla_instances(Bugzilla& bugzilla, MicroTask& app, API& api);

	void write_task_time_entry(const Task& task);
	void write_sessions(const Task& task);

	void write_bugzilla_group_by(const BugzillaInstance& instance);
	void write_bugzilla_bug_to_task(const BugzillaInstance& instance);

	SQLite::Database m_database;
};