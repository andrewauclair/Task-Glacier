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

	// get the field values from bugzilla
	if (m_curl)
	{
		m_bugzilla[info.name].fields.clear();

		std::string request = info.URL + "/rest/field/bug";

		auto result = m_curl->execute_request(request);

		simdjson::dom::parser parser;
		auto json = simdjson::padded_string(result);
		simdjson::dom::element doc = parser.parse(json);

		for (auto field : doc["fields"])
		{
			auto name = std::string(std::string_view(field["name"]));

			std::vector<std::string> values;

			for (auto value : field["values"])
			{
				values.emplace_back(std::string_view(value["name"]));
			}

			m_bugzilla[info.name].fields[name] = values;
		}

		// now go through all the group by tasks and construct our helper tasks
		build_group_by_task(m_bugzilla[info.name], app, api, output, m_bugzilla[info.name].bugzillaRootTaskID, m_bugzilla[info.name].bugzillaGroupTasksBy);
	}
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
			const auto result = app.create_task(value, parent, true);

			task = app.find_task(result.value());

			api.send_task_info(*task, true, output);
		}

		if (!groupTaskBy.empty())
		{
			build_group_by_task(instance, app, api, output, task->taskID(), groupTaskBy);
		}
	}
}

Task* Bugzilla::parent_task_for_bug(BugzillaInstance& instance, MicroTask& app, const simdjson::dom::element& bug, TaskID currentParent, std::span<const std::string> groupTaskBy)
{
	auto field = groupTaskBy[0];
	groupTaskBy = groupTaskBy.subspan(1);

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

	if (groupTaskBy.empty())
	{
		return task;
	}
	return parent_task_for_bug(instance, app, bug, task->taskID(), groupTaskBy);
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
			std::string request = info.bugzillaURL + "/rest/bug?assigned_to=" + info.bugzillaUsername + "&resolution=---&api_key=" + info.bugzillaApiKey;

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

			for (auto bug : doc["bugs"])
			{
				// check if we already have a task ID for this bug

				// find the parent for this task
				Task* parent = parent_task_for_bug(m_bugzilla[name], app, bug, m_bugzilla[name].bugzillaRootTaskID, m_bugzilla[name].bugzillaGroupTasksBy);

				// TODO what if parent is null? can it be?
				auto id = bug["id"];
				int i = id.get_int64();

				auto iter  = info.bugToTaskID.find(i);

				if (iter != info.bugToTaskID.end())
				{

				}
				else
				{
					const auto result = app.create_task(std::format("{} - {}", i, std::string_view(bug["summary"])), parent->taskID(), true);

					auto* task = app.find_task(result.value());

					api.send_task_info(*task, true, output);

					//info.bugToTaskID[i] = task->taskID();
					info.bugToTaskID.emplace(i, task->taskID());
				}
			}

			if (initial_refresh)
			{
				// just adding a bunch of new tasks
				for (auto bug : doc["bugs"])
				{
					TaskID parentID = NO_PARENT;
					/*simdjson::dom::element temp = bug[info.bugzillaGroupTasksBy];

					std::string groupBy;

					if (temp.is_array() && temp.get_array().size() > 0)
					{
						groupBy = temp.get_array().at(0);
					}
					else if (!temp.is_array())
					{
						groupBy = std::string(std::string_view(temp));
					}*/

					/*if (info.bugzillaGroupBy.contains(groupBy))
					{
						parentID = info.bugzillaGroupBy.at(groupBy);
					}
					else
					{
						parentID = app.create_task(groupBy, info.bugzillaRootTaskID, true).value();
						info.bugzillaGroupBy.emplace(groupBy, parentID);

						auto* task = app.find_task(parentID);

						api.send_task_info(*task, true, output);
					}

					auto id = bug["id"];
					std::int64_t i = id.get_int64();
					const auto result = app.create_task(std::format("{} - {}", i, std::string_view(bug["summary"])), parentID, true);

					auto* task = app.find_task(result.value());

					api.send_task_info(*task, true, output);

					info.bugzillaTasks.emplace(static_cast<std::int32_t>(i), result.value());*/
				}
			}
			else
			{
				// extra work to figure out if the task already exists. this will involve searching somehow for the bug
				for (auto bug : doc["bugs"])
				{
					TaskID parentID = NO_PARENT;
					/*simdjson::dom::element temp = bug[info.bugzillaGroupTasksBy];

					std::string groupBy;

					if (temp.is_array() && temp.get_array().size() > 0)
					{
						groupBy = temp.get_array().at(0);
					}
					else if (!temp.is_array())
					{
						groupBy = std::string(std::string_view(temp));
					}*/

					/*if (info.bugzillaGroupBy.contains(groupBy))
					{
						parentID = info.bugzillaGroupBy.at(groupBy);
					}
					else
					{
						parentID = app.create_task(groupBy, info.bugzillaRootTaskID, true).value();
						info.bugzillaGroupBy.emplace(groupBy, parentID);

						auto* task = app.find_task(parentID);

						api.send_task_info(*task, true, output);
					}

					auto id = bug["id"];
					std::int64_t i = id.get_int64();

					if (info.bugzillaTasks.contains(i))
					{
						auto* task = app.find_task(info.bugzillaTasks.at(i));

						if (parentID != task->parentID())
						{
							app.reparent_task(info.bugzillaTasks.at(i), parentID);
						}

						task->m_name = std::format("{} - {}", i, std::string_view(bug["summary"]));

						api.send_task_info(*task, false, output);
					}
					else
					{
						const auto result = app.create_task(std::format("{} - {}", i, std::string_view(bug["summary"])), parentID, true);

						auto* task = app.find_task(result.value());

						api.send_task_info(*task, true, output);

						info.bugzillaTasks.emplace(static_cast<std::int32_t>(i), result.value());
					}*/
				}

				/*for (auto&& parent : info.bugzillaGroupBy)
				{
					auto* task = app.find_task(parent.second);

					if (!app.task_has_children(parent.second))
					{
						app.finish_task(parent.second);
					}
					else if (task->state == TaskState::FINISHED)
					{
						task->state = TaskState::INACTIVE;
					}
					else
					{
						continue;
					}

					api.send_task_info(*task, false, output);
				}*/
			}

			info.lastBugzillaRefresh = now;

			file << "bugzilla-refresh " << info.bugzillaName << ' ' << info.lastBugzillaRefresh->count() << std::endl;

			//file << "bugzilla-tasks " << info.bugzillaName
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

	const int groupByCount = std::stoi(temp);

	for (int i = 0; i < groupByCount; i++)
	{
		std::string groupBy;
		std::getline(input, groupBy);

		bugzilla.bugzillaGroupTasksBy.push_back(groupBy);
	}

	std::getline(input, temp);

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

void Bugzilla::load_refresh(const std::string& line)
{
	auto values = split(line, ' ');

	BugzillaInstance& bugzilla = m_bugzilla[values[1]];

	bugzilla.lastBugzillaRefresh = std::chrono::milliseconds(std::stoll(values[2]));
	m_lastBugzillaRefresh = bugzilla.lastBugzillaRefresh;
}