#pragma once

#include "packets.hpp"

class Task;
struct BugzillaInstance;
class Bugzilla;
class MicroTask;
class API;
struct TaskTimes;

struct Database
{
	virtual bool database_exists(const std::string& file) = 0;

	virtual void create_database(const std::string& file) = 0;

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
};
