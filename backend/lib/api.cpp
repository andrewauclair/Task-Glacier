#include "api.hpp"

#include <iostream>

void API::process_packet(const Message& message, std::vector<std::unique_ptr<Message>>& output)
{
	switch (message.packetType())
	{
	case PacketType::CREATE_TASK:
		create_task(static_cast<const CreateTaskMessage&>(message), output);
		break;
	case PacketType::START_TASK:
		start_task(static_cast<const TaskMessage&>(message), output);
		break;
	case PacketType::STOP_TASK:
		stop_task(static_cast<const TaskMessage&>(message), output);
		break;
	case PacketType::FINISH_TASK:
		finish_task(static_cast<const TaskMessage&>(message), output);
		break;
	case PacketType::UPDATE_TASK:
		update_task(static_cast<const UpdateTaskMessage&>(message), output);
		break;
	case PacketType::REQUEST_TASK:
		request_task(static_cast<const TaskMessage&>(message), output);
		break;
	case PacketType::REQUEST_CONFIGURATION:
		handle_basic(static_cast<const BasicMessage&>(message), output);
		break;
	case PacketType::REQUEST_DAILY_REPORT:
	{
		const auto& request = static_cast<const RequestDailyReportMessage&>(message);

		create_daily_report(request.requestID, request.month, request.day, request.year, output);
		break;
	}
	case PacketType::TIME_CATEGORIES_MODIFY:
		time_categories_modify(static_cast<const TimeCategoriesModify&>(message), output);
		break;
	case PacketType::TIME_CATEGORIES_REQUEST:
		break;
	case PacketType::BUGZILLA_INFO:
	{
		const auto& info = static_cast<const BugzillaInfoMessage&>(message);

		m_bugzillaURL = info.URL;
		m_bugzillaKey = info.apiKey;
		m_bugzillaUsername = info.username;

		*m_output << "bugzilla: " << m_bugzillaURL << ' ' << m_bugzillaKey << '\n';
		*m_output << m_bugzillaUsername << '\n';
		*m_output << info.rootTaskID._val << '\n';
		*m_output << info.groupTasksBy << '\n';
		*m_output << info.labelToField.size() << '\n';

		for (auto&& f : info.labelToField)
		{
			*m_output << f.first << '\n' << f.second << '\n';
		}

		break;
	}
	case PacketType::BUGZILLA_REFRESH:
	{
		if (m_curl)
		{
			const bool initial_refresh = !m_lastBugzillaRefresh.has_value();

			// find all bugs that are not resolved
			// TODO find only bugs that have changed since the last refresh
			// TODO special processing for the initial refresh
			std::string request = m_bugzillaURL + "/rest/bug?assigned_to=" + m_bugzillaUsername + "&resolution=---&api_key=" + m_bugzillaKey;

			if (!initial_refresh)
			{
				// YYYY-MM-DDTHH24:MI:SSZ
				const auto ms = m_clock->now();
				auto time = std::chrono::system_clock::time_point(ms);

				std::chrono::year_month_day ymd = std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(time) };
				std::chrono::hh_mm_ss hms = std::chrono::hh_mm_ss{ ms };

				request += "&last_modified_time=" + std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z", (int)ymd.year(), (unsigned int)ymd.month(), (unsigned int)ymd.day(), hms.hours().count(), hms.minutes().count(), hms.seconds().count());
			}
			auto result = m_curl->execute_request(request);

			std::cout << result << '\n';
		}
		break;
	}
	}
}

void API::create_task(const CreateTaskMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	const auto result = m_app.create_task(message.name, message.parentID);

	if (result)
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = m_app.find_task(result.value());

		TaskInfoMessage info(task->taskID(), task->parentID(), task->m_name);
		info.newTask = true;
		info.state = task->state;
		info.createTime = task->createTime();

		output.push_back(std::make_unique<TaskInfoMessage>(info));
	}
	else
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.error()));
	}
}

void API::start_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	auto* currentActiveTask = m_app.active_task();

	const auto result = m_app.start_task(message.taskID);

	if (result)
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		if (currentActiveTask)
		{
			TaskInfoMessage info(currentActiveTask->taskID(), currentActiveTask->parentID(), currentActiveTask->m_name);
			info.state = currentActiveTask->state;
			info.createTime = currentActiveTask->createTime();
			info.times.insert(info.times.end(), currentActiveTask->m_times.begin(), currentActiveTask->m_times.end());

			output.push_back(std::make_unique<TaskInfoMessage>(info));
		}

		auto* task = m_app.find_task(message.taskID);

		send_task_info(*task, output);
	}
}

void API::stop_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	const auto result = m_app.stop_task(message.taskID);

	if (result)
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = m_app.find_task(message.taskID);

		send_task_info(*task, output);
	}
}

void API::finish_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	const auto result = m_app.finish_task(message.taskID);

	if (result)
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = m_app.find_task(message.taskID);

		send_task_info(*task, output);
	}
}

void API::update_task(const UpdateTaskMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	auto* task = m_app.find_task(message.taskID);

	if (!task)
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Task with ID {} does not exist.", message.taskID)));
		return;
	}

	std::optional<std::string> result;
	if (message.name != task->m_name)
	{
		result = m_app.rename_task(message.taskID, message.name);
	}
	else if (message.parentID != task->parentID())
	{
		result = m_app.reparent_task(message.taskID, message.parentID);
	}
	else
	{
		// TODO failure
	}
	

	if (result)
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		

		send_task_info(*task, output);
	}
}

void API::request_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	const auto* task = m_app.find_task(message.taskID);

	if (task)
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		send_task_info(*task, output);
	}
	else
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Task with ID {} does not exist.", message.taskID)));
	}
}

void API::handle_basic(const BasicMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	if (message.packetType() == PacketType::REQUEST_CONFIGURATION)
	{
		const auto send_task = [&](const Task& task) { send_task_info(task, output); };

		m_app.for_each_task_sorted(send_task);

		output.push_back(std::make_unique<BasicMessage>(PacketType::REQUEST_CONFIGURATION_COMPLETE));
	}
}

void API::send_task_info(const Task& task, std::vector<std::unique_ptr<Message>>& output)
{
	auto info = std::make_unique<TaskInfoMessage>(task.taskID(), task.parentID(), task.m_name);

	info->state = task.state;
	info->createTime = task.createTime();
	info->finishTime = task.m_finishTime;
	auto times = task.m_times;
	info->times = std::vector<TaskTimes>(times.begin(), times.end());

	output.push_back(std::move(info));
}

void API::time_categories_modify(const TimeCategoriesModify& message, std::vector<std::unique_ptr<Message>>& output)
{
	for (auto&& category : message.timeCategories)
	{
		if (category.id == TimeCategoryID(0))
		{
			// creating new time category
			// TODO check if one with the given name already exists

			auto newCategory = category;
			newCategory.id = TimeCategoryID(1);

			m_timeCategories.push_back(newCategory);
		}
	}
	output.push_back(std::make_unique<SuccessResponse>(message.requestID));

	TimeCategoriesData data;

	for (auto&& category : m_timeCategories)
	{
		//TimeCategoryPacket packet }
		//data.timeCategories.emplace_back();
	}
	output.push_back(std::make_unique<TimeCategoriesData>(data));
}

void API::create_daily_report(RequestID requestID, int month, int day, int year, std::vector<std::unique_ptr<Message>>& output)
{
	auto report = std::make_unique<DailyReportMessage>(requestID);

	// search for tasks on the given day

	std::vector<MicroTask::FindTasksOnDay> tasks = m_app.find_tasks_on_day(month, day, year);

	report->reportFound = !tasks.empty();

	if (report->reportFound)
	{
		report->report = { month, day, year };

		bool first = true;
		for (auto&& task : tasks)
		{
			if (first)
			{
				report->report.startTime = task.task->m_times[task.time.startStopIndex].start;
			}
			else if (report->report.startTime > task.task->m_times[task.time.startStopIndex].start)
			{
				report->report.startTime = task.task->m_times[task.time.startStopIndex].start;
			}
			first = false;
		}
	}

	output.push_back(std::move(report));
}
