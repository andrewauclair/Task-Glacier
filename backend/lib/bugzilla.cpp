#include "bugzilla.hpp"
#include "api.hpp"

#include <format>
#include <memory>
#include <string>

void Bugzilla::receive_info(const BugzillaInfoMessage& info, MicroTask& app, API& api, std::vector<std::unique_ptr<Message>>& output, std::ostream& file)
{
	m_bugzilla[info.name].bugzillaName = info.name;
	m_bugzilla[info.name].bugzillaURL = info.URL;
	m_bugzilla[info.name].bugzillaApiKey = info.apiKey;
	m_bugzilla[info.name].bugzillaUsername = info.username;
	m_bugzilla[info.name].bugzillaRootTaskID = info.rootTaskID;
	m_bugzilla[info.name].bugzillaGroupTasksBy = info.groupTasksBy;
	m_bugzilla[info.name].bugzillaLabelToField = info.labelToField;

	file << "bugzilla-config " << m_bugzilla[info.name].bugzillaName << ' ' << m_bugzilla[info.name].bugzillaURL << ' ' << m_bugzilla[info.name].bugzillaApiKey << '\n';
	file << m_bugzilla[info.name].bugzillaUsername << '\n';
	file << m_bugzilla[info.name].bugzillaRootTaskID._val << '\n';

	file << m_bugzilla[info.name].bugzillaGroupTasksBy.size() << '\n';

	for (auto&& f : info.groupTasksBy)
	{
		file << f << '\n';
	}

	file << m_bugzilla[info.name].bugzillaLabelToField.size() << '\n';

	for (auto&& f : info.labelToField)
	{
		file << f.first << '\n' << f.second << '\n';
	}

	file.flush();

	send_info(output);

	// get the field values from bugzilla
	if (m_curl)
	{
		m_bugzilla[info.name].fields.clear();

		std::string request = info.URL + "/rest/field/bug?api_key=" + info.apiKey;

		auto result = m_curl->execute_request(request);

		simdjson::dom::parser parser;
		auto json = simdjson::padded_string(result);
		simdjson::dom::element doc = parser.parse(json);

		for (auto field : doc["fields"])
		{
			auto name = std::string(std::string_view(field["name"]));

			std::vector<std::string> values;

			if (field.at_key("values").error() != simdjson::error_code::NO_SUCH_FIELD)
			{
				for (auto value : field["values"])
				{
					try
					{
						values.emplace_back(std::string_view(value["name"]));
					}
					catch (const simdjson::simdjson_error& ignore)
					{
					}
				}
			}

			m_bugzilla[info.name].fields[name] = values;
		}

		std::vector<TaskID> bugTasks;

		for (auto&& [bug, task] : m_bugzilla[info.name].bugToTaskID)
		{
			bugTasks.push_back(task);
		}

		std::map<TaskID, TaskState> helperTasks;
		app.find_bugzilla_helper_tasks(m_bugzilla[info.name].bugzillaRootTaskID, bugTasks, helperTasks);

		// now go through all the group by tasks and construct our helper tasks
		if (!m_bugzilla[info.name].bugzillaGroupTasksBy.empty())
		{
			build_group_by_task(m_bugzilla[info.name], app, api, output, m_bugzilla[info.name].bugzillaRootTaskID, m_bugzilla[info.name].bugzillaGroupTasksBy);
		}

		std::map<TaskID, TaskState> helperTasks2;
		app.find_bugzilla_helper_tasks(m_bugzilla[info.name].bugzillaRootTaskID, bugTasks, helperTasks2);


		// finish all the non-bug children of the root task. we'll find the parents and set them back to inactive as we need them
		
		for (auto&& [id, state] : helperTasks2)
		{
			Task* task = app.find_task(id);

			const auto result = helperTasks.find(id);

			if (result != helperTasks.end() && state != result->second)
			{
				api.send_task_info(*task, false, output);
			}
			else if (result == helperTasks.end())
			{
				app.finish_task(id);

				api.send_task_info(*task, true, output);
			}
		}
	}

	RequestMessage request(PacketType::BUGZILLA_REFRESH, RequestID(0));

	refresh(request, app, api, file, output);
}

void Bugzilla::build_group_by_task(BugzillaInstance& instance, MicroTask& app, API& api, std::vector<std::unique_ptr<Message>>& output, TaskID parent, std::span<const std::string> groupTaskBy)
{
	auto field = groupTaskBy[0];
	groupTaskBy = groupTaskBy.subspan(1);

	for (auto&& value : instance.fields[field])
	{
		Task* task = app.find_task_with_parent_and_name(value, parent);

		if (!task)
		{
			// make sure the parent task isn't finished
			if (app.find_task(parent) != nullptr)
			{
				app.find_task(parent)->state = TaskState::INACTIVE;
			}

			const auto result = app.create_task(value, parent, true);

			if (result)
			{
				task = app.find_task(result.value());
			}
			else
			{
				int breakpoint = 0;
			}
		}

		if (task && !groupTaskBy.empty())
		{
			build_group_by_task(instance, app, api, output, task->taskID(), groupTaskBy);
		}
	}
}

Task* Bugzilla::parent_task_for_bug(BugzillaInstance& instance, MicroTask& app, API& api, std::vector<std::unique_ptr<Message>>& output, const simdjson::dom::element& bug, TaskID currentParent, std::span<const std::string> groupTaskBy)
{
	auto field = groupTaskBy[0];
	groupTaskBy = groupTaskBy.subspan(1);

	if (bug.at_key(field).error() == simdjson::error_code::NO_SUCH_FIELD)
	{
		return nullptr;
	}
	simdjson::dom::element temp = bug[field];

	std::string groupBy;

	if (temp.is_array() && temp.get_array().size() > 0)
	{
		groupBy = temp.get_array().at(0);
	}
	else if (!temp.is_array())
	{
		groupBy = std::string(std::string_view(temp));
	}
	
	Task* task = app.find_task_with_parent_and_name(groupBy, currentParent);

	if (!task)
	{
		// make sure the parent task isn't finished
		if (app.find_task(currentParent) != nullptr)
		{
			app.find_task(currentParent)->state = TaskState::INACTIVE;
		}

		const auto result = app.create_task(groupBy, currentParent, true);

		task = app.find_task(result.value());

		api.send_task_info(*task, true, output);
	}

	if (groupTaskBy.empty())
	{
		return task;
	}
	return parent_task_for_bug(instance, app, api, output, bug, task->taskID(), groupTaskBy);
}

void Bugzilla::send_info(std::vector<std::unique_ptr<Message>>& output)
{
	for (auto&& [name, info] : m_bugzilla)
	{
		auto bugzilla = BugzillaInfoMessage(info.bugzillaName, info.bugzillaURL, info.bugzillaApiKey);
		bugzilla.username = info.bugzillaUsername;
		bugzilla.rootTaskID = info.bugzillaRootTaskID;
		bugzilla.groupTasksBy = info.bugzillaGroupTasksBy;
		bugzilla.labelToField = info.bugzillaLabelToField;

		output.push_back(std::make_unique<BugzillaInfoMessage>(bugzilla));
	}
}

void Bugzilla::refresh(const RequestMessage& request, MicroTask& app, API& api, std::ostream& file, std::vector<std::unique_ptr<Message>>& output)
{
	if (m_curl)
	{
		const auto now = m_clock->now();

		for (auto&& [name, info] : m_bugzilla)
		{
			if (app.find_task(info.bugzillaRootTaskID) == nullptr)
			{
				output.push_back(std::make_unique<FailureResponse>(request.requestID, std::format("Root task {} does not exist", info.bugzillaRootTaskID)));
				return;
			}
		}

		// TODO eventually we'll have to send back a failure if we weren't able to contact bugzilla
		output.push_back(std::make_unique<SuccessResponse>(request.requestID));

		for (auto&& [name, info] : m_bugzilla)
		{
			const bool initial_refresh = !info.lastBugzillaRefresh.has_value();

			// find all bugs that are not resolved
			// TODO find only bugs that have changed since the last refresh
			// TODO special processing for the initial refresh
			std::string requestAddress = info.bugzillaURL + "/rest/bug?assigned_to=" + info.bugzillaUsername + "&api_key=" + info.bugzillaApiKey;

			// not the initial refresh and not an internal request. only get latest changes
			if (!initial_refresh && request.requestID != RequestID(0))
			{
				// YYYY-MM-DDTHH24:MI:SSZ

				auto time = std::chrono::system_clock::time_point(now);

				std::chrono::year_month_day ymd = std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(time) };
				std::chrono::hh_mm_ss hms = std::chrono::hh_mm_ss{ now - m_clock->midnight() };

				requestAddress += "&last_change_time=" + std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z", (int)ymd.year(), (unsigned int)ymd.month(), (unsigned int)ymd.day(), hms.hours().count(), hms.minutes().count(), hms.seconds().count());
			}
			else
			{
				requestAddress += "&resolution=---";
			}

			auto result = m_curl->execute_request(requestAddress);

			simdjson::dom::parser parser;
			auto json = simdjson::padded_string(result);
			simdjson::dom::element doc = parser.parse(json);

			std::vector<TaskID> bugTasks;
			std::map<TaskID, TaskState> startingStates;

			for (auto&& [bug, task] : info.bugToTaskID)
			{
				bugTasks.push_back(task);
				startingStates[task] = app.find_task(task)->state;
			}

			// finish all the non-bug children of the root task. we'll find the parents and set them back to inactive as we need them
			std::map<TaskID, TaskState> helperTasks;
			app.find_bugzilla_helper_tasks(info.bugzillaRootTaskID, bugTasks, helperTasks);

			for (auto bug : doc["bugs"])
			{
				// check if we already have a task ID for this bug

				// find the parent for this task
				Task* parent = app.find_task(info.bugzillaRootTaskID);

				if (!m_bugzilla[name].bugzillaGroupTasksBy.empty())
				{
					parent = parent_task_for_bug(m_bugzilla[name], app, api, output, bug, m_bugzilla[name].bugzillaRootTaskID, m_bugzilla[name].bugzillaGroupTasksBy);
				}

				[&]()
					{
						Task* nextParent = parent;

						while (nextParent)
						{
							nextParent->state = TaskState::INACTIVE;
							nextParent->m_finishTime = std::nullopt;

							file << "unfinish " << nextParent->taskID()._val << '\n';

							nextParent = app.find_task(nextParent->parentID());
						}
					}();

				auto id = bug["id"];
				int i = id.get_int64();

				auto iter  = info.bugToTaskID.find(i);

				if (iter != info.bugToTaskID.end())
				{
					auto name = std::format("{} - {}", i, std::string_view(bug["summary"]));

					Task* task = app.find_task(iter->second);

					bool sendInfo = false;

					if (task && name != task->m_name)
					{
						app.rename_task(iter->second, name);

						sendInfo = true;
					}

					if (task && parent && parent->taskID() != task->parentID())
					{
						app.reparent_task(task->taskID(), parent->taskID());

						sendInfo = true;
					}

					if (task && std::string_view(bug["status"]) == "RESOLVED")
					{
						app.finish_task(task->taskID());

						sendInfo = true;
					}

					if (sendInfo)
					{
						api.send_task_info(*task, false, output);
					}
				}
				else if (parent)
				{
					const auto result = app.create_task(std::format("{} - {}", i, std::string_view(bug["summary"])), parent->taskID(), true);

					auto* task = app.find_task(result.value());

					api.send_task_info(*task, true, output);

					//info.bugToTaskID[i] = task->taskID();
					info.bugToTaskID.emplace(i, task->taskID());
				}
			}

			bugTasks.clear();
			startingStates.clear();

			for (auto&& [bug, task] : info.bugToTaskID)
			{
				bugTasks.push_back(task);
				startingStates[task] = app.find_task(task)->state;
			}

			for (auto&& [taskID, state] : helperTasks)
			{
				Task* task = app.find_task(taskID);

				if (!app.task_has_bug_tasks(taskID, bugTasks))
				{
					app.finish_task(taskID);
				}

				if (task && task->state != state)
				{
					api.send_task_info(*task, false, output);
				}
			}

			info.lastBugzillaRefresh = now;

			file << "bugzilla-refresh " << info.bugzillaName << ' ' << info.lastBugzillaRefresh->count() << std::endl;

			file << "bugzilla-tasks " << info.bugzillaName << ' ';

			for (auto&& [bug, task] : info.bugToTaskID)
			{
				file << bug << ' ' << task._val << ' ';
			}
			file << std::endl;
		}
		m_lastBugzillaRefresh = now;
	}
}

void Bugzilla::load_config(const std::string& line, std::istream& input)
{
	auto values = split(line, ' ');

	BugzillaInstance& bugzilla = m_bugzilla[values[1]];

	bugzilla.bugzillaName = values[1];
	bugzilla.bugzillaURL = values[2];
	bugzilla.bugzillaApiKey = values[3];
	std::getline(input, bugzilla.bugzillaUsername);

	std::string temp;
	std::getline(input, temp);

	bugzilla.bugzillaRootTaskID = TaskID(std::stoi(temp));

	std::getline(input, temp);

	bugzilla.bugzillaGroupTasksBy.clear();

	const int groupByCount = std::stoi(temp);

	for (int i = 0; i < groupByCount; i++)
	{
		std::string groupBy;
		std::getline(input, groupBy);

		bugzilla.bugzillaGroupTasksBy.push_back(groupBy);
	}

	std::getline(input, temp);

	bugzilla.bugzillaLabelToField.clear();

	const int labelCount = std::stoi(temp);

	for (int i = 0; i < labelCount; i++)
	{
		std::string label;
		std::string field;
		std::getline(input, label);
		std::getline(input, field);

		bugzilla.bugzillaLabelToField[label] = field;
	}
}

void Bugzilla::load_refresh(const std::string& line, const std::string& tasks)
{
	auto values = split(line, ' ');

	BugzillaInstance& bugzilla = m_bugzilla[values[1]];

	bugzilla.lastBugzillaRefresh = std::chrono::milliseconds(std::stoll(values[2]));
	m_lastBugzillaRefresh = bugzilla.lastBugzillaRefresh;

	values = split(tasks, ' ');

	bugzilla.bugToTaskID.clear();

	for (int i = 2; i < values.size() - 1; i += 2)
	{
		int bug = std::stoi(values[i]);
		auto task = TaskID(std::stoi(values[i + 1]));

		bugzilla.bugToTaskID.emplace(bug, task);
	}
}