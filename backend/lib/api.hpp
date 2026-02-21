#ifndef MICRO_TASK_API_HPP
#define MICRO_TASK_API_HPP

#include "server.hpp"
#include "curl.hpp"
#include "bugzilla.hpp"
#include "database.hpp"

#include "packets/create_task.hpp"
#include "packets/task.hpp"
#include "packets/update_task.hpp"
#include "packets/time_entry_modify_packet.hpp"

#include <vector>

#include "packet_sender.hpp"

class API
{
public:
	API(const Clock& clock, cURL& curl, Database& database, PacketSender& sender) 
		: m_clock(&clock), 
		m_app(*this, clock, database, sender),
		m_bugzilla(clock, curl, sender),
		m_database(&database),
		m_sender(&sender)
	{
		database.load(m_bugzilla, m_app, *this);
	}

	void process_packet(const Message& message);

	void send_task_info(const Task& task, bool newTask);

private:
	void create_task(const CreateTaskMessage& message);
	void start_task(const TaskMessage& message);
	void stop_task(const TaskMessage& message);
	void stop_unspecified_task(const TaskMessage& message);
	void finish_task(const TaskMessage& message);
	void update_task(const UpdateTaskMessage& message);
	void request_task(const TaskMessage& message);

	void handle_basic(const BasicMessage& message);

	void time_entry_modify(const TimeEntryModifyPacket& message);

	DailyReportMessage create_daily_report(RequestOrigin request, int month, int day, int year);
	void create_weekly_report(RequestOrigin request, int month, int day, int year);

	const Clock* m_clock;
	
public:
	MicroTask m_app;
	Bugzilla m_bugzilla;
private:
	Database* m_database;
	PacketSender* m_sender;
};

#endif
