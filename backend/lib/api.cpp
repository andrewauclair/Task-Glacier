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

		m_app.m_bugzillaURL = info.URL;
		m_app.m_bugzillaApiKey = info.apiKey;
		m_app.m_bugzillaUsername = info.username;
		m_app.m_bugzillaRootTaskID = info.rootTaskID;
		m_app.m_bugzillaGroupTasksBy = info.groupTasksBy;
		m_app.m_bugzillaLabelToField = info.labelToField;

		*m_output << "bugzilla-config " << m_app.m_bugzillaURL << ' ' << m_app.m_bugzillaApiKey << '\n';
		*m_output << m_app.m_bugzillaUsername << '\n';
		*m_output << m_app.m_bugzillaRootTaskID._val << '\n';
		*m_output << m_app.m_bugzillaGroupTasksBy << '\n';
		*m_output << m_app.m_bugzillaLabelToField.size() << '\n';

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
			const auto now = m_clock->now();

			const auto& refresh = static_cast<const RequestMessage&>(message);

			// TODO eventually we'll have to send back a failure if we weren't able to contact bugzilla
			output.push_back(std::make_unique<SuccessResponse>(refresh.requestID));

			const bool initial_refresh = !m_app.m_lastBugzillaRefresh.has_value();

			// find all bugs that are not resolved
			// TODO find only bugs that have changed since the last refresh
			// TODO special processing for the initial refresh
			std::string request = m_app.m_bugzillaURL + "/rest/bug?assigned_to=" + m_app.m_bugzillaUsername + "&resolution=---&api_key=" + m_app.m_bugzillaApiKey;

			if (!initial_refresh)
			{
				// YYYY-MM-DDTHH24:MI:SSZ
				
				auto time = std::chrono::system_clock::time_point(now);
				 
				std::chrono::year_month_day ymd = std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(time) };
				std::chrono::hh_mm_ss hms = std::chrono::hh_mm_ss{ now - m_clock->midnight() };

				request += "&last_change_time=" + std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z", (int)ymd.year(), (unsigned int)ymd.month(), (unsigned int)ymd.day(), hms.hours().count(), hms.minutes().count(), hms.seconds().count());
			}

			auto result = m_curl->execute_request(request);

			simdjson::dom::parser parser;
			auto json = simdjson::padded_string(result);
			simdjson::dom::element doc = parser.parse(json);

			if (initial_refresh)
			{
				// just adding a bunch of new tasks
				for (auto bug : doc["bugs"])
				{
					TaskID parentID = NO_PARENT;
					simdjson::dom::element temp = bug[m_app.m_bugzillaGroupTasksBy];
					
					std::string groupBy;

					if (temp.is_array() && temp.get_array().size() > 0)
					{
						groupBy = temp.get_array().at(0);
					}
					else if (!temp.is_array())
					{
						groupBy = std::string(std::string_view(temp));
					}

					if (m_app.m_bugzillaGroupBy.contains(groupBy))
					{
						parentID = m_app.m_bugzillaGroupBy.at(groupBy);
					}
					else
					{
						parentID = m_app.create_task(groupBy, m_app.m_bugzillaRootTaskID).value();
						m_app.m_bugzillaGroupBy.emplace(groupBy, parentID);

						auto* task = m_app.find_task(parentID);

						send_task_info(*task, true, output);
					}

					auto id = bug["id"];
					std::int64_t i = id.get_int64();
					const auto result = m_app.create_task(std::format("{} - {}", i, std::string_view(bug["summary"])), parentID);

					auto* task = m_app.find_task(result.value());

					send_task_info(*task, true, output);

					m_app.m_bugzillaTasks.emplace(static_cast<std::int32_t>(i), result.value());
				}
			}
			else
			{
				// extra work to figure out if the task already exists. this will involve searching somehow for the bug
				for (auto bug : doc["bugs"])
				{
					TaskID parentID = NO_PARENT;
					simdjson::dom::element temp = bug[m_app.m_bugzillaGroupTasksBy];

					std::string groupBy;

					if (temp.is_array() && temp.get_array().size() > 0)
					{
						groupBy = temp.get_array().at(0);
					}
					else if (!temp.is_array())
					{
						groupBy = std::string(std::string_view(temp));
					}

					if (m_app.m_bugzillaGroupBy.contains(groupBy))
					{
						parentID = m_app.m_bugzillaGroupBy.at(groupBy);
					}
					else
					{
						parentID = m_app.create_task(groupBy, m_app.m_bugzillaRootTaskID).value();
						m_app.m_bugzillaGroupBy.emplace(groupBy, parentID);

						auto* task = m_app.find_task(parentID);

						send_task_info(*task, true, output);
					}

					auto id = bug["id"];
					std::int64_t i = id.get_int64();

					if (m_app.m_bugzillaTasks.contains(i))
					{
						auto* task = m_app.find_task(m_app.m_bugzillaTasks.at(i));

						if (parentID != task->parentID())
						{
							m_app.reparent_task(m_app.m_bugzillaTasks.at(i), parentID);
						}

						task->m_name = std::format("{} - {}", i, std::string_view(bug["summary"]));

						send_task_info(*task, false, output);
					}
				}

				for (auto&& parent : m_app.m_bugzillaGroupBy)
				{
					auto* task = m_app.find_task(parent.second);

					if (!m_app.task_has_children(parent.second))
					{
						m_app.finish_task(parent.second);
					}
					else if (task->state == TaskState::FINISHED)
					{
						task->state = TaskState::INACTIVE;
					}
					else
					{
						continue;
					}

					send_task_info(*task, false, output);
				}
			}

			m_app.m_lastBugzillaRefresh = now;

			*m_output << "bugzilla-refresh " << m_app.m_lastBugzillaRefresh->count() << '\n';
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
		auto* task = m_app.find_task(result.value());

		m_app.configure_task_time_codes(task->taskID(), message.timeCodes);

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

		auto bugzilla = BugzillaInfoMessage(m_app.m_bugzillaURL, m_app.m_bugzillaApiKey);
		bugzilla.username = m_app.m_bugzillaUsername;
		bugzilla.rootTaskID = m_app.m_bugzillaRootTaskID;
		bugzilla.groupTasksBy = m_app.m_bugzillaGroupTasksBy;
		bugzilla.labelToField = m_app.m_bugzillaLabelToField;

		output.push_back(std::make_unique<BugzillaInfoMessage>(bugzilla));

		TimeCategoriesData data({});

		for (auto&& category : m_timeCategories)
		{
			TimeCategory packet = TimeCategory(category.id, category.name);

			for (auto&& code : category.codes)
			{
				TimeCode codePacket = TimeCode(code.id, code.name);

				packet.codes.push_back(codePacket);
			}
			data.timeCategories.push_back(packet);
		}
		output.push_back(std::make_unique<TimeCategoriesData>(data));

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
	info->times = task.m_times;
	info->timeCodes = task.timeCodes;

	output.push_back(std::move(info));
}

void API::time_categories_modify(const TimeCategoriesModify& message, std::vector<std::unique_ptr<Message>>& output)
{
	for (auto&& category : message.timeCategories)
	{
		TimeCategory* timeCategory = nullptr;

		if (category.id == TimeCategoryID(0) && message.type == TimeCategoryModType::ADD)
		{
			// creating new time category
			auto result = std::find_if(m_timeCategories.begin(), m_timeCategories.end(), [&](auto&& cat) { return cat.name == category.name; });

			if (result != m_timeCategories.end())
			{
				output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Time Category with name '{}' already exists", category.name)));
				return;
			}

			TimeCategory newCategory{ m_nextTimeCategoryID, category.name };

			m_nextTimeCategoryID++;

			m_timeCategories.push_back(newCategory);

			timeCategory = &m_timeCategories.back();
		}
		else
		{
			auto result = std::find_if(m_timeCategories.begin(), m_timeCategories.end(), [&](auto&& cat) { return cat.id == category.id;});

			if (result != m_timeCategories.end())
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
					output.push_back(std::make_unique<FailureResponse>(message.requestID, std::format("Time Code with ID {} does not exist", code.id)));
					return;
				}
			}
		}
		else if (message.type == TimeCategoryModType::REMOVE_CATEGORY)
		{
			auto result = std::find_if(m_timeCategories.begin(), m_timeCategories.end(), [&](auto&& cat) { return cat.id == category.id; });

			m_timeCategories.erase(result);
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
						copyCode.id = m_nextTimeCodeID;

						m_nextTimeCodeID++;

						timeCategory->codes.push_back(copyCode);
					}
				}
			}
		}
		
	}
	output.push_back(std::make_unique<SuccessResponse>(message.requestID));

	TimeCategoriesData data({});

	for (auto&& category : m_timeCategories)
	{
		TimeCategory packet = TimeCategory(category.id, category.name);

		for (auto&& code : category.codes)
		{
			TimeCode codePacket = TimeCode(code.id, code.name);

			packet.codes.push_back(codePacket);
		}
		data.timeCategories.push_back(packet);
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
			report->report.times.emplace_back(task.task->taskID(), task.time.startStopIndex);

			if (first)
			{
				report->report.startTime = task.task->m_times[task.time.startStopIndex].start;
			}
			else if (report->report.startTime > task.task->m_times[task.time.startStopIndex].start)
			{
				report->report.startTime = task.task->m_times[task.time.startStopIndex].start;
			}

			if (task.task->m_times[task.time.startStopIndex].stop.has_value())
			{
				const auto timeForTask = task.task->m_times[task.time.startStopIndex].stop.value() - task.task->m_times[task.time.startStopIndex].start;

				report->report.totalTime += timeForTask;

				for (auto&& timeCode : task.task->timeCodes)
				{
					report->report.timePerTimeCode[timeCode] += timeForTask;
				}
			}

			if (task.task->m_times[task.time.startStopIndex].stop.has_value() && task.task->m_times[task.time.startStopIndex].stop.value() > report->report.endTime)
			{
				report->report.endTime = task.task->m_times[task.time.startStopIndex].stop.value();
			}
			first = false;
		}
	}

	output.push_back(std::move(report));
}
