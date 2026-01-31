#include "api.hpp"
#include "simdjson.h"

#include "packets/request_daily_report.hpp"
#include "packets/request_weekly_report.hpp"
#include "packets/success_response.hpp"
#include "packets/failure_response.hpp"
#include "packets/time_entry.hpp"
#include "packets/weekly_report.hpp"
#include "packets/time_entry_data_packet.hpp"

#include <iostream>

#include "packets/update_task_times.hpp"
#include "packets/version.hpp"

void API::process_packet(const Message& message)
{
	switch (message.packetType())
	{
	case PacketType::VERSION_REQUEST:
		m_sender->send(std::make_unique<VersionMessage>("0.13.0"));
		break;
	case PacketType::CREATE_TASK:
		create_task(static_cast<const CreateTaskMessage&>(message));
		break;
	case PacketType::START_TASK:
	case PacketType::START_UNSPECIFIED_TASK:
		start_task(static_cast<const TaskMessage&>(message));
		break;
	case PacketType::STOP_TASK:
		stop_task(static_cast<const TaskMessage&>(message));
		break;
	case PacketType::STOP_UNSPECIFIED_TASK:
		stop_unspecified_task(static_cast<const TaskMessage&>(message));
		break;
	case PacketType::FINISH_TASK:
		finish_task(static_cast<const TaskMessage&>(message));
		break;
	case PacketType::UPDATE_TASK:
		update_task(static_cast<const UpdateTaskMessage&>(message));
		break;
	case PacketType::ADD_TASK_SESSION:
	{
		const auto& update = static_cast<const UpdateTaskTimesMessage&>(message);

		auto* task = m_app.find_task(update.taskID);

		if (!task)
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, std::format("Task with ID {} does not exist.", update.taskID)));
			break;
		}

		if (!update.stop.has_value())
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, "New session must have a stop time."));
			break;
		}

		if (update.stop.has_value() && update.stop <= update.start)
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, "Stop time cannot be before start time."));
			break;
		}

		const Task* overlap_task = nullptr;
		const TaskTimes* overlap_time = nullptr;

		const auto detect = [update, &overlap_task, &overlap_time](const Task& task)
		{
			if (overlap_task)
			{
				return;
			}

			for (const TaskTimes& times : task.m_times)
			{
				bool overlap_detected = false;

				if (update.start >= times.start && (!times.stop.has_value() || update.start <= times.stop.value()))
				{
					overlap_detected = true;
				}
				else if (times.start >= update.start && times.start <= update.stop.value())
				{
					overlap_detected = true;
				}
				else if (update.stop.value() >= times.start && update.stop.value() <= times.stop.value())
				{
					overlap_detected = true;
				}

				if (overlap_detected)
				{
					overlap_task = &task;
					overlap_time = &times;
					break;
				}
			}
		};

		m_app.for_each_task_sorted(detect);

		if (overlap_task)
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, std::format("Overlap detected with '{}'.", overlap_task->m_name)));
			break;
		}

		if (update.checkForOverlaps)
		{
			m_sender->send(std::make_unique<SuccessResponse>(update.requestID));
		}
		else
		{
			task->m_times.push_back(TaskTimes{ update.start, update.stop });

			m_app.fill_session_time_entry(*task, task->m_times.back());

			std::sort(task->m_times.begin(), task->m_times.end());

			m_database->write_task(*task, *m_sender);

			m_sender->send(std::make_unique<SuccessResponse>(update.requestID));
			send_task_info(*task, false);
		}
		break;
	}
	case PacketType::EDIT_TASK_SESSION:
	{
		const auto& update = static_cast<const UpdateTaskTimesMessage&>(message);

		auto* task = m_app.find_task(update.taskID);

		if (!task)
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, std::format("Task with ID {} does not exist.", update.taskID)));
			break;
		}

		if (update.sessionIndex >= task->m_times.size())
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, "Invalid session index."));
			break;
		}

		if (update.stop.has_value() && update.stop <= update.start)
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, "Stop time cannot be before start time."));
			break;
		}

		TaskTimes& times = task->m_times.at(update.sessionIndex);

		if (times.stop.has_value() && !update.stop.has_value())
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, "Cannot remove stop time."));
			break;
		}
		else if (!times.stop.has_value() && update.stop.has_value())
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, "Cannot add stop time."));
			break;
		}

		const Task* overlap_task = nullptr;
		const TaskTimes* overlap_time = nullptr;

		const auto detect = [task, update, &overlap_task, &overlap_time](const Task& other_task)
		{
			if (task == &other_task)
			{
				return;
			}
			if (overlap_task)
			{
				return;
			}

			for (const TaskTimes& times : other_task.m_times)
			{
				bool overlap_detected = false;

				if (update.start >= times.start && (!times.stop.has_value() || update.start <= times.stop.value()))
				{
					overlap_detected = true;
				}
				else if (times.start >= update.start && times.start <= update.stop.value())
				{
					overlap_detected = true;
				}
				else if (update.stop.value() >= times.start && update.stop.value() <= times.stop.value())
				{
					overlap_detected = true;
				}

				if (overlap_detected)
				{
					overlap_task = &other_task;
					overlap_time = &times;
					break;
				}
			}
		};

		m_app.for_each_task_sorted(detect);

		if (overlap_task)
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, std::format("Overlap detected with '{}'.", overlap_task->m_name)));
			break;
		}

		if (update.checkForOverlaps)
		{
			m_sender->send(std::make_unique<SuccessResponse>(update.requestID));
		}
		else
		{
			times.start = update.start;
			times.stop = update.stop;

			m_database->write_task(*task, *m_sender);

			m_sender->send(std::make_unique<SuccessResponse>(update.requestID));
			send_task_info(*task, false);
		}
		break;
	}
	case PacketType::REMOVE_TASK_SESSION:
	{
		const auto& update = static_cast<const UpdateTaskTimesMessage&>(message);

		auto* task = m_app.find_task(update.taskID);

		if (!task)
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, std::format("Task with ID {} does not exist.", update.taskID)));
			break;
		}

		if (update.sessionIndex >= static_cast<std::int32_t>(task->m_times.size()))
		{
			m_sender->send(std::make_unique<FailureResponse>(update.requestID, "Invalid session index."));
			break;
		}

		task->m_times.erase(task->m_times.begin() + update.sessionIndex);

		m_database->remove_sessions(task->taskID(), *m_sender);

		m_database->write_task(*task, *m_sender);

		m_sender->send(std::make_unique<SuccessResponse>(update.requestID));
		send_task_info(*task, false);

		break;
	}
	case PacketType::REQUEST_TASK:
		request_task(static_cast<const TaskMessage&>(message));
		break;
	case PacketType::REQUEST_CONFIGURATION:
	case PacketType::BULK_TASK_UPDATE_START:
	case PacketType::BULK_TASK_UPDATE_FINISH:
		handle_basic(static_cast<const BasicMessage&>(message));
		break;
	case PacketType::REQUEST_DAILY_REPORT:
	{
		const auto& request = static_cast<const RequestDailyReportMessage&>(message);

		auto report = create_daily_report(request.requestID, request.month, request.day, request.year);

		m_sender->send(std::make_unique<DailyReportMessage>(report));

		break;
	}
	case PacketType::REQUEST_WEEKLY_REPORT:
	{
		const auto& request = static_cast<const RequestWeeklyReportMessage&>(message);

		create_weekly_report(request.requestID, request.month, request.day, request.year);
		break;
	}
	case PacketType::TIME_ENTRY_MODIFY:
		time_entry_modify(static_cast<const TimeEntryModifyPacket&>(message));
		break;
	case PacketType::TIME_ENTRY_REQUEST:
		break;
	case PacketType::BUGZILLA_INFO:
	{
		const auto& info = static_cast<const BugzillaInfoMessage&>(message);

		m_bugzilla.receive_info(info, m_app, *this, *m_database);

		break;
	}
	case PacketType::BUGZILLA_REFRESH:
	{
		const auto& request = static_cast<const RequestMessage&>(message);

		m_bugzilla.perform_refresh(request, m_app, *this, *m_database);

		break;
	}
	}
}

void API::create_task(const CreateTaskMessage& message)
{
	const auto result = m_app.create_task(message.name, message.parentID);

	if (result)
	{
		auto* task = m_app.find_task(result.value());

		m_app.configure_task_time_entry(task->taskID(), message.timeEntry); 

		m_sender->send(std::make_unique<SuccessResponse>(message.requestID));

		send_task_info(*task, true);
	}
	else
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, result.error()));
	}
}

void API::start_task(const TaskMessage& message)
{
	auto* currentActiveTask = m_app.active_task();

	if (currentActiveTask && currentActiveTask->taskID() == UNSPECIFIED_TASK)
	{
		std::string failure;
		if (message.taskID == UNSPECIFIED_TASK)
		{
			failure = "Unspecified task is already active.";
		}
		else
		{
			failure = std::format("Cannot start task with ID {}. Unspecified task is active.", message.taskID);
		}
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, failure));

		return;
	}

	const auto result = m_app.start_task(message.taskID);

	if (result)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		m_sender->send(std::make_unique<SuccessResponse>(message.requestID));

		if (currentActiveTask)
		{
			send_task_info(*currentActiveTask, false);
		}

		auto* task = m_app.find_task(message.taskID);

		// don't send the unspecified task
		if (task->taskID() != UNSPECIFIED_TASK)
		{
			send_task_info(*task, false);
		}
		else
		{
			m_sender->send(std::make_unique<BasicMessage>(PacketType::UNSPECIFIED_TASK_ACTIVE));
		}
	}
}

void API::stop_task(const TaskMessage& message)
{
	if (message.taskID == UNSPECIFIED_TASK)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, "Unspecified task cannot be stopped."));

		return;
	}

	const auto result = m_app.stop_task(message.taskID);

	if (result)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		m_sender->send(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = m_app.find_task(message.taskID);

		send_task_info(*task, false);
	}
}

void API::stop_unspecified_task(const TaskMessage& message)
{
	if (message.packetType() == PacketType::STOP_TASK)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, "Unspecified task cannot be stopped."));

		return;
	}

	if (m_app.find_task(message.taskID) == nullptr)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, std::format("Task with ID {} does not exist.", message.taskID)));

		return;
	}

	auto* currentActiveTask = m_app.active_task();

	if (!currentActiveTask)
	{
		return;
	}

	(void) m_app.stop_task(UNSPECIFIED_TASK);

	const auto result = m_app.start_task(message.taskID, currentActiveTask->m_times.back().start);

	if (result)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		m_sender->send(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = m_app.find_task(message.taskID);

		send_task_info(*task, false);
	}
}

void API::finish_task(const TaskMessage& message)
{
	if (message.taskID == UNSPECIFIED_TASK && message.packetType() == PacketType::FINISH_TASK)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, "Unspecified task cannot be finished."));

		return;
	}

	const auto result = m_app.finish_task(message.taskID);

	if (result)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		m_sender->send(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = m_app.find_task(message.taskID);

		send_task_info(*task, false);
	}
}

void API::update_task(const UpdateTaskMessage& message)
{
	auto* task = m_app.find_task(message.taskID);

	if (!task)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, std::format("Task with ID {} does not exist.", message.taskID)));

		return;
	}

	bool indexChanged = task->indexInParent != message.indexInParent;
	bool parentChanged = task->parentID() != message.parentID;
	TaskID currentParent = task->parentID();

	task->locked = message.locked;
	task->indexInParent = message.indexInParent;

	std::optional<std::string> result;

	if (message.name != task->m_name)
	{
		result = m_app.rename_task(message.taskID, message.name);
	}
	else if (message.parentID != task->parentID())
	{
		result = m_app.reparent_task(message.taskID, message.parentID);
	}
	else // assume time entry changed
	{
		// TODO validation of time codes, make sure they exist
		task->m_times = message.times;
		task->timeEntry = message.timeEntry;

		m_database->write_task(*task, *m_sender);
	}
	
	if (result)
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		if (indexChanged)
		{
			// find all tasks for the parent and fix the index values, keeping the current task as is
			std::vector<Task*> children = m_app.find_tasks_with_parent(task->parentID());
			std::sort(children.begin(), children.end(), [&](Task* a, Task* b) { 
				if (a->indexInParent == b->indexInParent)
				{
					if (a == task) return true;
					if (b == task) return false;
				}
				return a->indexInParent < b->indexInParent; 
			});

			int expectedIndex = 0;

			for (Task* child : children)
			{
				if (child->indexInParent != expectedIndex)
				{
					child->indexInParent = expectedIndex;
					if (!m_app.is_bulk_update())
					{
						send_task_info(*child, false);
					}
					else
					{
						m_app.add_update_task(child->taskID());
					}
					m_database->write_task(*child, *m_sender);
				}
				++expectedIndex;
			}
		}

		m_sender->send(std::make_unique<SuccessResponse>(message.requestID));

		if (!m_app.is_bulk_update())
		{
			send_task_info(*task, false);
		}
		else
		{
			m_app.add_update_task(task->taskID());
		}
	}
}

void API::request_task(const TaskMessage& message)
{
	const auto* task = m_app.find_task(message.taskID);

	if (task)
	{
		m_sender->send(std::make_unique<SuccessResponse>(message.requestID));

		send_task_info(*task, false);
	}
	else
	{
		m_sender->send(std::make_unique<FailureResponse>(message.requestID, std::format("Task with ID {} does not exist.", message.taskID)));
	}
}

void API::handle_basic(const BasicMessage& message)
{
	if (message.packetType() == PacketType::REQUEST_CONFIGURATION)
	{
		TimeEntryDataPacket data({});

		auto& time_categories = m_app.timeCategories();
		
		for (auto&& category : time_categories)
		{
			TimeCategory packet = TimeCategory(category.id, category.name);

			for (auto&& code : category.codes)
			{
				TimeCode codePacket = TimeCode(code.id, code.name);

				packet.codes.push_back(codePacket);
			}
			data.timeCategories.push_back(packet);
		}
		m_sender->send(std::make_unique<TimeEntryDataPacket>(data));

		/*const auto send_task = [&](const Task& task) { send_task_info(task, false, output); };

		m_app.for_each_task_sorted(send_task);*/
		m_app.send_all_tasks();

		m_bugzilla.send_info();

		m_sender->send(std::make_unique<BasicMessage>(PacketType::REQUEST_CONFIGURATION_COMPLETE));
	}
	else if (message.packetType() == PacketType::BULK_TASK_UPDATE_START)
	{
		// pause any task updates until we receive the finish
		// this message will be followed by the task updates, all using the same request ID
		m_app.start_bulk_update();
		m_database->start_transaction(*m_sender);
	}
	else if (message.packetType() == PacketType::BULK_TASK_UPDATE_FINISH)
	{
		// now send the task update for any tasks that changed
		m_app.finish_bulk_update();
		m_database->finish_transaction(*m_sender);
	}
}

void API::send_task_info(const Task& task, bool newTask)
{
	auto info = std::make_unique<TaskInfoMessage>(task.taskID(), task.parentID(), task.m_name);

	info->state = task.state;
	info->createTime = task.createTime();
	info->finishTime = task.m_finishTime;
	info->newTask = newTask;
	info->indexInParent = task.indexInParent;
	info->serverControlled = task.serverControlled;
	info->locked = task.locked;
	info->times = task.m_times;
	info->timeEntry = task.timeEntry;
	info->labels = task.labels;

	m_sender->send(std::move(info));
}

void API::time_entry_modify(const TimeEntryModifyPacket& message)
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
				m_sender->send(std::make_unique<FailureResponse>(message.requestID, std::format("Time Category with name '{}' already exists", category.name)));
				return;
			}

			TimeCategory newCategory{ m_app.m_nextTimeCategoryID, category.name };

			m_app.m_nextTimeCategoryID++;

			m_database->write_next_time_category_id(m_app.m_nextTimeCategoryID, *m_sender);

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
				m_sender->send(std::make_unique<FailureResponse>(message.requestID, std::format("Time Category with ID {} does not exist", category.id)));
				return;
			}
		}

		if (timeCategory == nullptr)
		{
			return;
		}

		if (message.type == TimeCategoryModType::UPDATE)
		{
			// update names
			timeCategory->name = category.name;

			for (auto&& code : category.codes)
			{
				auto result = std::find_if(timeCategory->codes.begin(), timeCategory->codes.end(), [&](auto&& c) { return c.id == code.id; });

				if (result != timeCategory->codes.end())
				{
					result->name = code.name;
				}
				else
				{
					m_sender->send(std::make_unique<FailureResponse>(message.requestID, std::format("Time Code with ID {} does not exist", code.id)));
					return;
				}
			}

			m_database->write_time_entry_config(*timeCategory, *m_sender);
		}
		else if (message.type == TimeCategoryModType::REMOVE_CATEGORY)
		{
			auto result = std::find_if(m_app.timeCategories().begin(), m_app.timeCategories().end(), [&](auto&& cat) { return cat.id == category.id; });

			if (result != m_app.timeCategories().end())
			{
				m_database->remove_time_category(*result, *m_sender);
			}

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
					m_sender->send(std::make_unique<FailureResponse>(message.requestID, std::format("Time Code with ID {} does not exist", code.id)));
					return;
				}
			}

			for (auto&& code : category.codes)
			{
				// TODO validate that this is called multiple times in storage_test.cpp
				m_database->remove_time_code(category, code, *m_sender);
			}
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
						m_sender->send(std::make_unique<FailureResponse>(message.requestID, std::format("Time Code with name '{}' already exists on Time Category '{}'", code.name, timeCategory->name)));
						return;
					}
					else
					{
						auto copyCode = code;
						copyCode.id = m_app.m_nextTimeCodeID;

						m_app.m_nextTimeCodeID++;

						timeCategory->codes.push_back(copyCode);

						m_database->write_next_time_code_id(m_app.m_nextTimeCodeID, *m_sender);
					}
				}
			}

			m_database->write_time_entry_config(*timeCategory, *m_sender);
		}
		
	}
	m_sender->send(std::make_unique<SuccessResponse>(message.requestID));

	TimeEntryDataPacket data({});

	for (auto&& category : m_app.timeCategories())
	{
		TimeCategory packet = TimeCategory(category.id, category.name);

		for (auto&& code : category.codes)
		{
			TimeCode codePacket = TimeCode(code.id, code.name);

			packet.codes.push_back(codePacket);
		}
		data.timeCategories.push_back(packet);
	}
	m_sender->send(std::make_unique<TimeEntryDataPacket>(data));
}

DailyReportMessage API::create_daily_report(RequestID requestID, int month, int day, int year)
{
	DailyReportMessage report(requestID, m_clock->now());

	// search for tasks on the given day

	std::vector<MicroTask::FindTasksOnDay> tasks = m_app.find_tasks_on_day(month, day, year);

	report.report = { !tasks.empty(), month, day, year };

	if (report.report.found)
	{
		bool first = true;

		std::sort(tasks.begin(), tasks.end(),
			[](MicroTask::FindTasksOnDay& a, MicroTask::FindTasksOnDay& b)
			{
				return a.task->taskID() < b.task->taskID();
			}
		);

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

void API::create_weekly_report(RequestID requestID, int month, int day, int year)
{
	WeeklyReportMessage report(requestID, m_clock->now());

	report.dailyReports[0] = create_daily_report(requestID, month, day, year).report;

	auto ymd = std::chrono::year_month_day(std::chrono::year(year), std::chrono::month(month), std::chrono::day(day));

	auto days = static_cast<std::chrono::local_days>(ymd);

	for (int i = 1; i < 7; i++)
	{
		days++;

		auto f = std::chrono::year_month_day(days);

		report.dailyReports[i] = create_daily_report(requestID, static_cast<unsigned int>(f.month()), static_cast<unsigned int>(f.day()), static_cast<int>(f.year())).report;
	}

	m_sender->send(std::make_unique<WeeklyReportMessage>(report));
}