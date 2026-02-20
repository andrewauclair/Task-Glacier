#pragma once

#include "packets/task_id.hpp"
#include "packets/bugzilla_instance_id.hpp"
#include "packets/time_category_id.hpp"
#include "packets/time_code_id.hpp"
#include "packets/time_code.hpp"

#include <SQLiteCpp/Database.h>

#include "packet_sender.hpp"


struct BugzillaInstance;
struct TaskTimes;
struct TimeCategory;

class Task;
class Bugzilla;
class MicroTask;
class API;

struct Database
{
	virtual ~Database() = default;

	virtual void load(Bugzilla& bugzilla, MicroTask& app, API& api) = 0;

	// write task
	virtual void write_task(const Task& task, PacketSender& sender) = 0;
	virtual void write_next_task_id(TaskID nextID, PacketSender& sender) = 0;

	// write bugzilla config
	virtual void write_bugzilla_instance(const BugzillaInstance& instance, PacketSender& sender) = 0;
	virtual void write_next_bugzilla_instance_id(BugzillaInstanceID nextID, PacketSender& sender) = 0;
	virtual void remove_bugzilla_instance(int ID) = 0;
	virtual void bugzilla_refreshed(int ID) = 0;

	// write time entry configuration
	// write sessions
	virtual void write_session(TaskID task, const TaskTimes& session, PacketSender& sender) = 0;
	virtual void remove_sessions(TaskID task, PacketSender& sender) = 0;

	// write time entries
	virtual void write_time_entry(TaskID task, PacketSender& sender) = 0;
	virtual void remove_time_entry() = 0;

	virtual void write_time_entry_config(const TimeCategory& entry, PacketSender& sender) = 0;
	virtual void write_next_time_category_id(TimeCategoryID nextID, PacketSender& sender) = 0;
	virtual void write_next_time_code_id(TimeCodeID nextID, PacketSender& sender) = 0;

	virtual void remove_time_category(const TimeCategory& entry, PacketSender& sender) = 0;
	virtual void remove_time_code(const TimeCategory& entry, const TimeCode& code, PacketSender& sender) = 0;

	virtual void start_transaction(PacketSender& sender) = 0;
	virtual void finish_transaction(PacketSender& sender) = 0;

	virtual bool transaction_in_progress() const = 0;
};

struct DatabaseImpl : Database
{
	DatabaseImpl(const std::string& file, PacketSender& sender);

	SQLite::Database& database() { return m_database; }

	void load(Bugzilla& bugzilla, MicroTask& app, API& api) override;

	// write task
	void write_task(const Task& task, PacketSender& sender) override;
	void write_next_task_id(TaskID nextID, PacketSender& sender) override;

	// write bugzilla config
	void write_bugzilla_instance(const BugzillaInstance& instance, PacketSender& sender) override;
	void write_next_bugzilla_instance_id(BugzillaInstanceID nextID, PacketSender& sender) override;
	void remove_bugzilla_instance(int ID) override {}
	void bugzilla_refreshed(int ID) override {}

	// write time entry configuration
	// write sessions
	void write_session(TaskID task, const TaskTimes& session, PacketSender& sender) override {}
	void remove_sessions(TaskID task, PacketSender& sender) override;

	// write time entries
	void write_time_entry(TaskID task, PacketSender& sender) override {}
	void remove_time_entry() override {}

	void write_time_entry_config(const TimeCategory& entry, PacketSender& sender) override;
	void write_next_time_category_id(TimeCategoryID nextID, PacketSender& sender) override;
	void write_next_time_code_id(TimeCodeID nextID, PacketSender& sender) override;

	void remove_time_category(const TimeCategory& entry, PacketSender& sender) override;
	void remove_time_code(const TimeCategory& entry, const TimeCode& code, PacketSender& sender) override;

	void start_transaction(PacketSender& sender) override;
	void finish_transaction(PacketSender& sender) override;

	bool transaction_in_progress() const override;

private:
	void load_time_entry(MicroTask& app);
	void load_tasks(MicroTask& app);
	void load_bugzilla_instances(Bugzilla& bugzilla, MicroTask& app);
	void load_next_ids(Bugzilla& bugzilla, MicroTask& app);

	void write_task_time_entry(const Task& task, PacketSender& sender);
	void write_sessions(const Task& task, PacketSender& sender);

	void write_bugzilla_group_by(const BugzillaInstance& instance, PacketSender& sender);
	void write_bugzilla_bug_to_task(const BugzillaInstance& instance, PacketSender& sender);

	bool execute_statement(SQLite::Statement& statement, PacketSender& sender);

	SQLite::Database m_database;
	bool m_transaction_in_progress = false;
};