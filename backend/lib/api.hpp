#ifndef MICRO_TASK_API_HPP
#define MICRO_TASK_API_HPP

#include "server.hpp"
#include "packets.hpp"
#include "curl.hpp"

#include <vector>

class API
{
public:
	API(const Clock& clock, std::istream& input, std::ostream& output) : m_clock(&clock), m_app(clock, output), m_output(&output)
	{
		m_app.load_from_file(input);
	}

	API(const Clock& clock, cURL& curl, std::istream& input, std::ostream& output) : m_clock(&clock), m_curl(&curl), m_app(clock, output), m_output(&output)
	{
		m_app.load_from_file(input);
	}

	void process_packet(const Message& message, std::vector<std::unique_ptr<Message>>& output);

private:
	void create_task(const CreateTaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void start_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void stop_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void finish_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void update_task(const UpdateTaskMessage& message, std::vector<std::unique_ptr<Message>>& output);
	void request_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output);

	void handle_basic(const BasicMessage& message, std::vector<std::unique_ptr<Message>>& output);

	void send_task_info(const Task& task, std::vector<std::unique_ptr<Message>>& output);

	void time_categories_modify(const TimeCategoriesModify& message, std::vector<std::unique_ptr<Message>>& output);

	void create_daily_report(RequestID requestID, int month, int day, int year, std::vector<std::unique_ptr<Message>>& output);

	std::string m_bugzillaURL;
	std::string m_bugzillaKey;
	std::string m_bugzillaUsername;

	const Clock* m_clock;
	cURL* m_curl = nullptr;
	MicroTask m_app;
	std::ostream* m_output;
};

#endif
