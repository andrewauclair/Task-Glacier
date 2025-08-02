#ifndef MICRO_TASK_API_HPP
#define MICRO_TASK_API_HPP

#include "server.hpp"
#include "packets.hpp"
#include "curl.hpp"
#include "bugzilla.hpp"
#include "database.hpp"

#include <vector>

class API
{
public:
	API(const Clock& clock, cURL& curl, Database& database) 
		: m_clock(&clock), 
		m_app(*this, clock, database),
		m_bugzilla(clock, curl),
		m_database(&database)
	{
		database.load(m_bugzilla, m_app, *this);
	}

	void process_packet(const Message& message, std::vector<std::unique_ptr<Message>>& output);

	void send_task_info(const Task& task, bool newTask, std::vector<std::unique_ptr<Message>>& output);

private:
	void create_task(const CreateTaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void start_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void stop_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void finish_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void update_task(const UpdateTaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void request_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output);

	void handle_basic(const BasicMessage& message, std::vector<std::unique_ptr<Message>>& output);

	void time_entry_modify(const TimeEntryModifyPacket& message, std::vector<std::unique_ptr<Message>>& output);

	DailyReportMessage create_daily_report(RequestID requestID, int month, int day, int year);
	void create_weekly_report(RequestID requestID, int month, int day, int year, std::vector<std::unique_ptr<Message>>& output);

	const Clock* m_clock;
	MicroTask m_app;
public:
	Bugzilla m_bugzilla;
private:
	Database* m_database;
};

#endif
