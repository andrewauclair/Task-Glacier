#include "api.hpp"
#include "simdjson.h"

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

		auto report = create_daily_report(request.requestID, request.month, request.day, request.year);

		output.push_back(std::make_unique<DailyReportMessage>(report));

		break;
	}
	case PacketType::REQUEST_WEEKLY_REPORT:
	{
		const auto& request = static_cast<const RequestWeeklyReportMessage&>(message);

		create_weekly_report(request.requestID, request.month, request.day, request.year, output);
		break;
	}
	case PacketType::TIME_ENTRY_MODIFY:
		time_entry_modify(static_cast<const TimeEntryModifyPacket&>(message), output);
		break;
	case PacketType::TIME_ENTRY_REQUEST:
		break;
	case PacketType::BUGZILLA_INFO:
	{
		const auto& info = static_cast<const BugzillaInfoMessage&>(message);

		m_bugzilla.receive_info(info, m_app, *this, output, *m_output);

		break;
	}
	case PacketType::BUGZILLA_REFRESH:
	{
		m_bugzilla.refresh(static_cast<const RequestMessage&>(message), m_app, *this, *m_output, output);
		break;
	}
	}
}

void API::create_task(const CreateTaskMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	const auto result = m_app.create_task(message.name, message.parentID);

	if (result)
	{
		auto* task = m_app.find_task(result.value());

		m_app.configure_task_time_entry(task->taskID(), message.timeEntry);

		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		send_task_info(*task, true, output);
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
			send_task_info(*currentActiveTask, false, output);
		}

		auto* task = m_app.find_task(message.taskID);

		send_task_info(*task, false, output);
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

		send_task_info(*task, false, output);
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

		send_task_info(*task, false, output);
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

	// TODO test and persist
	task->locked = message.locked;

	std::optional<std::string> result;
	if (message.name != task->m_name)
	{
		result = m_app.rename_task(message.taskID, message.name);
	}
	else if (message.parentID != task->parentID())
	{
		result = m_app.reparent_task(message.taskID, message.parentID);
	}
	else if (message.timeEntry.size() != task->timeEntry.size())
	{
		// TODO validation of time codes, make sure they exist
		task->timeEntry = message.timeEntry;

		*m_output << "task-time-codes " << task->taskID()._val << ' ';

		if (task->timeEntry.empty())
		{
			*m_output << "0 ";
		}
		else
		{
			for (auto&& code : task->timeEntry)
			{
				*m_output << code.categoryID._val << ' ' << code.codeID._val << ' ';
			}
		}
		*m_output << std::endl;
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

		send_task_info(*task, false, output);
	}
}

void API::request_task(const TaskMessage& message, std::vector<std::unique_ptr<Message>>& output)
{
	const auto* task = m_app.find_task(message.taskID);

	if (task)
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		send_task_info(*task, false, output);
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
		const auto send_task = [&](const Task& task) { send_task_info(task, false, output); };

		m_app.for_each_task_sorted(send_task);

		m_bugzilla.send_info(output);

		TimeEntryDataPacket data({});

		for (auto&& category : m_app.timeCategories())
		{
			TimeCategory packet = TimeCategory(category.id, category.name, category.label);

			for (auto&& code : category.codes)
			{
				TimeCode codePacket = TimeCode(code.id, code.name);

				packet.codes.push_back(codePacket);
			}
			data.timeCategories.push_back(packet);
		}
		output.push_back(std::make_unique<TimeEntryDataPacket>(data));

		output.push_back(std::make_unique<BasicMessage>(PacketType::REQUEST_CONFIGURATION_COMPLETE));
	}
}

void API::send_task_info(const Task& task, bool newTask, std::vector<std::unique_ptr<Message>>& output)
{
	auto info = std::make_unique<TaskInfoMessage>(task.taskID(), task.parentID(), task.m_name);

	info->state = task.state;
	info->createTime = task.createTime();
	info->finishTime = task.m_finishTime;
	info->newTask = newTask;
	info->serverControlled = task.serverControlled;
	info->locked = task.locked;
	info->times = task.m_times;
	info->timeEntry = task.timeEntry;
	info->labels = task.labels;

	output.push_back(std::move(info));
}

void API::time_entry_modify(const TimeEntryModifyPacket& message, std::vector<std::unique_ptr<Message>>& output)
{
	for (auto&& category : message.timeCategories)
	{
		TimeCategory* timeCategory = nullptr;

		if (category.id == TimeCategoryID(0) && message.type == TimeCategoryModType::ADD)
		{
			// creating new time category
			auto result = std::find_if(m_app.timeCategories().begin(), m_app.timeCategories().end(), [&](auto&& cat) { return cat.name == category.name; });

			if (result != m_app.timeCategories().end())
			{
				output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Time Category with name '{}' already exists", category.name)));
				return;
			}

			TimeCategory newCategory{ m_app.m_nextTimeCategoryID, category.name, category.label };

			m_app.m_nextTimeCategoryID++;

			m_app.timeCategories().push_back(newCategory);

			timeCategory = &m_app.timeCategories().back();
		}
		else
		{
			auto result = std::find_if(m_app.timeCategories().begin(), m_app.timeCategories().end(), [&](auto&& cat) { return cat.id == category.id;});

			if (result != m_app.timeCategories().end())
			{
				timeCategory = &(*result);
			}
			else
			{
				// failed to find a time category with the given ID
				output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Time Category with ID {} does not exist", category.id)));
				return;
			}
		}

		if (timeCategory == nullptr)
		{
			return;
		}

		// TODO persist to file so we can reload next time

		if (message.type == TimeCategoryModType::UPDATE)
		{
			// update names
			timeCategory->name = category.name;
			timeCategory->label = category.label;

			for (auto&& code : category.codes)
			{
				auto result = std::find_if(timeCategory->codes.begin(), timeCategory->codes.end(), [&](auto&& c) { return c.id == code.id; });

				if (result != timeCategory->codes.end())
				{
					result->name = code.name;
				}
				else
				{
					output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Time Code with ID {} does not exist", code.id)));
					return;
				}
			}

			*m_output << "time-category update " << timeCategory->id._val << ' ' << persist_string(timeCategory->name) << ' ';
			*m_output << persist_string(timeCategory->label) << ' ';

			*m_output << category.codes.size() << ' ';

			for (auto&& code : timeCategory->codes)
			{
				*m_output << code.id._val << ' ' << persist_string(code.name) << ' ';
			}
			*m_output << std::endl;
		}
		else if (message.type == TimeCategoryModType::REMOVE_CATEGORY)
		{
			auto result = std::find_if(m_app.timeCategories().begin(), m_app.timeCategories().end(), [&](auto&& cat) { return cat.id == category.id; });

			if (result != m_app.timeCategories().end())
			{
				*m_output << "time-category remove-category " << result->id._val << '\n';
			}

			m_output->flush();

			m_app.timeCategories().erase(result);
		}
		else if (message.type == TimeCategoryModType::REMOVE_CODE)
		{
			for (auto&& code : category.codes)
			{
				auto result = std::find_if(timeCategory->codes.begin(), timeCategory->codes.end(), [&](auto&& c) { return c.id == code.id; });

				if (result != timeCategory->codes.end())
				{
					timeCategory->codes.erase(result);
				}
				else
				{
					output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Time Code with ID {} does not exist", code.id)));
					return;
				}
			}

			*m_output << "time-category remove-code " << category.id._val;

			*m_output << " { ";

			for (auto&& code : category.codes)
			{
				*m_output << code.id._val << ' ';
			}

			*m_output << "}";

			*m_output << std::endl;
		}
		else
		{
			bool allow = true;

			for (auto&& code : category.codes)
			{
				const bool newCode = code.id == TimeCodeID(0);

				if (newCode)
				{
					const auto existing = std::find_if(timeCategory->codes.begin(), timeCategory->codes.end(), [&](auto&& c) { return c.name == code.name; });

					if (existing != timeCategory->codes.end())
					{
						output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Time Code with name '{}' already exists on Time Category '{}'", code.name, timeCategory->name)));
						return;
					}
					else
					{
						auto copyCode = code;
						copyCode.id = m_app.m_nextTimeCodeID;

						m_app.m_nextTimeCodeID++;

						timeCategory->codes.push_back(copyCode);
					}
				}
			}

			*m_output << "time-category add " << category.id._val << ' ' << persist_string(category.name) << ' ';
			*m_output << persist_string(category.label) << ' ';

			*m_output << category.codes.size() << ' ';

			for (auto&& code : category.codes)
			{
				*m_output << code.id._val << ' ' << persist_string(code.name) << ' ';
			}
			*m_output << std::endl;
		}
		
	}
	output.push_back(std::make_unique<SuccessResponse>(message.requestID));

	TimeEntryDataPacket data({});

	for (auto&& category : m_app.timeCategories())
	{
		TimeCategory packet = TimeCategory(category.id, category.name, category.label);

		for (auto&& code : category.codes)
		{
			TimeCode codePacket = TimeCode(code.id, code.name);

			packet.codes.push_back(codePacket);
		}
		data.timeCategories.push_back(packet);
	}
	output.push_back(std::make_unique<TimeEntryDataPacket>(data));
}

DailyReportMessage API::create_daily_report(RequestID requestID, int month, int day, int year)
{
	DailyReportMessage report(requestID);

	// search for tasks on the given day

	std::vector<MicroTask::FindTasksOnDay> tasks = m_app.find_tasks_on_day(month, day, year);

	report.report = { !tasks.empty(), month, day, year };

	if (report.report.found)
	{
		bool first = true;
		
		for (auto&& task : tasks)
		{
			report.report.times.emplace_back(task.task->taskID(), task.time.startStopIndex);

			TaskTimes& times = task.task->m_times[task.time.startStopIndex];

			if (first)
			{
				report.report.startTime = times.start;
			}
			else if (report.report.startTime > times.start)
			{
				report.report.startTime = times.start;
			}

			if (times.stop.has_value())
			{
				const auto timeForTask = times.stop.value() - times.start;

				report.report.totalTime += timeForTask;

				if (times.timeEntry.empty())
				{
					//report.report.timePerTimeEntry[TimeCodeID(0)] += timeForTask;
				}

				for (auto&& timeCode : times.timeEntry)
				{
					report.report.timePerTimeEntry[timeCode] += timeForTask;
				}
			}
			else // task is still active
			{
				const auto timeForTask = m_clock->now() - times.start;

				report.report.totalTime += timeForTask;

				if (times.timeEntry.empty())
				{
					//report.report.timePerTimeEntry[TimeCodeID(0)] += timeForTask;
				}

				for (auto&& timeCode : times.timeEntry)
				{
					report.report.timePerTimeEntry[timeCode] += timeForTask;
				}
			}

			if (times.stop.has_value() && times.stop.value() > report.report.endTime)
			{
				report.report.endTime = times.stop.value();
			}
			first = false;
		}
	}

	return report;
}

void API::create_weekly_report(RequestID requestID, int month, int day, int year, std::vector<std::unique_ptr<Message>>& output)
{
	WeeklyReportMessage report(requestID);

	report.dailyReports[0] = create_daily_report(requestID, month, day, year).report;

	auto ymd = std::chrono::year_month_day(std::chrono::year(year), std::chrono::month(month), std::chrono::day(day));

	auto days = static_cast<std::chrono::local_days>(ymd);

	for (int i = 1; i < 7; i++)
	{
		days++;

		auto f = std::chrono::year_month_day(days);

		report.dailyReports[i] = create_daily_report(requestID, static_cast<unsigned int>(f.month()), static_cast<unsigned int>(f.day()), static_cast<int>(f.year())).report;
	}

	output.push_back(std::make_unique<WeeklyReportMessage>(report));
}