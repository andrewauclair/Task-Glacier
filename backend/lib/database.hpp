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
	bool database_exists(const std::string& file);

	void create_database(const std::string& file);

	void load(Bugzilla& bugzilla, MicroTask& app, API& api);

	// write task
	void write_task(const Task& task);
	void next_task_id(TaskID taskID);

	// write bugzilla config
	void write_bugzilla_instance(const BugzillaInstance& instance);
	void remove_bugzilla_instance(int ID);
	void next_bugzilla_instance_id(int ID);
	void bugzilla_refreshed(int ID);

	// write time entry configuration
	// write sessions
	void write_session(TaskID task, const TaskTimes& session);
	void remove_session();

	// write time entries
	void write_time_entry(TaskID task);
	void remove_time_entry();
};
