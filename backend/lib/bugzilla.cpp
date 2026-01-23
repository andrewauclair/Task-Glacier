#include "bugzilla.hpp"
#include "api.hpp"

#include "packets/success_response.hpp"
#include "packets/failure_response.hpp"

#include <format>
#include <memory>
#include <string>

void Bugzilla::receive_info(const BugzillaInfoMessage& info, MicroTask& app, API& api, Database& database)
{
	BugzillaInstance* instance = nullptr;

	if (info.instanceID._val == 0)
	{
		m_bugzilla.emplace(info.name, BugzillaInstance(m_nextBugzillaID));
		++m_nextBugzillaID;

		database.write_next_bugzilla_instance_id(m_nextBugzillaID, *m_sender);
	}
	
	instance = &m_bugzilla.at(info.name);

	instance->bugzillaName = info.name;
	instance->bugzillaURL = info.URL;
	instance->bugzillaApiKey = info.apiKey;
	instance->bugzillaUsername = info.username;
	instance->bugzillaRootTaskID = info.rootTaskID;
	instance->bugzillaGroupTasksBy = info.groupTasksBy;
	instance->bugzillaLabelToField = info.labelToField;

	database.write_bugzilla_instance(*instance, *m_sender);

	send_info();

	// get the field values from bugzilla
	if (m_curl)
	{
		m_bugzilla.at(info.name).fields.clear();

		std::string request = info.URL + "/rest/field/bug?api_key=" + info.apiKey;

		auto result = m_curl->execute_request(request);

		if (result)
		{
			simdjson::dom::parser parser;
			auto json = simdjson::padded_string(result.value());
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

				m_bugzilla.at(info.name).fields[name] = values;
			}
		}
	}

	RequestMessage request(PacketType::BUGZILLA_REFRESH, RequestID(0));

	refresh(request, app, api, database);
}

void Bugzilla::build_group_by_task(BugzillaInstance& instance, MicroTask& app, API& api, TaskID parent, std::span<const std::string> groupTaskBy)
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
				app.find_task(parent)->state = TaskState::PENDING;
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
			build_group_by_task(instance, app, api, task->taskID(), groupTaskBy);
		}
	}
}

Task* Bugzilla::parent_task_for_bug(BugzillaInstance& instance, MicroTask& app, API& api, const simdjson::dom::element& bug, TaskID currentParent, std::span<const std::string> groupTaskBy)
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
			app.find_task(currentParent)->state = TaskState::PENDING;
		}

		const auto result = app.create_task(groupBy, currentParent, true);

		task = app.find_task(result.value());

		api.send_task_info(*task, true);
	}

	if (groupTaskBy.empty())
	{
		return task;
	}
	return parent_task_for_bug(instance, app, api, bug, task->taskID(), groupTaskBy);
}

void Bugzilla::send_info()
{
	for (auto&& [name, info] : m_bugzilla)
	{
		auto bugzilla = BugzillaInfoMessage(info.instanceID, info.bugzillaName, info.bugzillaURL, info.bugzillaApiKey);
		bugzilla.username = info.bugzillaUsername;
		bugzilla.rootTaskID = info.bugzillaRootTaskID;
		bugzilla.groupTasksBy = info.bugzillaGroupTasksBy;
		bugzilla.labelToField = info.bugzillaLabelToField;

		m_sender->send(std::make_unique<BugzillaInfoMessage>(bugzilla));
	}
}

void Bugzilla::refresh(const RequestMessage& request, MicroTask& app, API& api, Database& database)
{
	if (m_curl)
	{
		const auto now = m_clock->now();

		for (auto&& [name, info] : m_bugzilla)
		{
			if (app.find_task(info.bugzillaRootTaskID) == nullptr)
			{
				if (request.requestID != RequestID(0))
				{
					m_sender->send(std::make_unique<FailureResponse>(request.requestID, std::format("Root task {} does not exist", info.bugzillaRootTaskID)));
				}
				return;
			}
		}

		// TODO eventually we'll have to send back a failure if we weren't able to contact bugzilla
		if (request.requestID != RequestID(0))
		{
			m_sender->send(std::make_unique<SuccessResponse>(request.requestID));
		}

		for (auto&& [instanceName, info] : m_bugzilla)
		{
			const bool initial_refresh = !info.lastBugzillaRefresh.has_value();

			const auto refresh = [&](const std::string& requestAddress) -> bool
			{
					auto result = m_curl->execute_request(requestAddress);

					if (!result) return false;

					simdjson::dom::parser parser;
					auto json = simdjson::padded_string(result.value());
					simdjson::dom::element doc = parser.parse(json);

					std::vector<TaskID> bugTasks;
					std::map<TaskID, TaskState> startingStates;

					for (auto&& [bug, task] : info.bugToTaskID)
					{
						bugTasks.push_back(task);
						startingStates[task] = app.find_task(task)->state;
					}

					// finish all the non-bug children of the root task. we'll find the parents and set them back to pending as we need them
					std::map<TaskID, TaskState> helperTasks;
					app.find_bugzilla_helper_tasks(info.bugzillaRootTaskID, bugTasks, helperTasks);

					for (auto bug : doc["bugs"])
					{
						// check if we already have a task ID for this bug

						// find the parent for this task
						Task* parent = app.find_task(info.bugzillaRootTaskID);

						if (!info.bugzillaGroupTasksBy.empty())
						{
							parent = parent_task_for_bug(info, app, api, bug, info.bugzillaRootTaskID, info.bugzillaGroupTasksBy);
						}

						[&]()
							{
								Task* nextParent = parent;

								while (nextParent)
								{
									nextParent->state = TaskState::PENDING;
									nextParent->m_finishTime = std::nullopt;

									nextParent = app.find_task(nextParent->parentID());
								}
							}();

						auto id = bug["id"];
						int i = id.get_int64();

						auto iter = info.bugToTaskID.find(i);

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
								api.send_task_info(*task, false);
							}
						}
						else if (parent)
						{
							const auto result = app.create_task(std::format("{} - {}", i, std::string_view(bug["summary"])), parent->taskID(), true);

							auto* task = app.find_task(result.value());

							api.send_task_info(*task, true);

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

						if (!app.task_has_active_bug_tasks(taskID, bugTasks))
						{
							app.finish_task(taskID);
						}

						if (task && task->state != state)
						{
							api.send_task_info(*task, false);
						}
					}
					return true;
			};

			// find all bugs that are not resolved
			std::string requestAddress = info.bugzillaURL + "/rest/bug?assigned_to=" + info.bugzillaUsername + "&api_key=" + info.bugzillaApiKey;

			// not the initial refresh and not an internal request. only get latest changes
			if (!initial_refresh && request.requestID != RequestID(0))
			{
				// YYYY-MM-DDTHH24:MI:SSZ

				auto last_refresh_time = info.lastBugzillaRefresh.value_or(std::chrono::milliseconds(0));

				auto time = std::chrono::system_clock::time_point(last_refresh_time);

				std::chrono::year_month_day ymd = std::chrono::year_month_day{ std::chrono::floor<std::chrono::days>(time) };
				std::chrono::hh_mm_ss hms = std::chrono::hh_mm_ss{ last_refresh_time - m_clock->midnight(time) };

				requestAddress += "&last_change_time=" + std::format("{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}Z", (int)ymd.year(), (unsigned int)ymd.month(), (unsigned int)ymd.day(), hms.hours().count(), hms.minutes().count(), hms.seconds().count());
			}
			else
			{
				requestAddress += "&resolution=---";
			}

			bool assignedSuccess = refresh(requestAddress);
			requestAddress.replace(requestAddress.find("assigned_to"), std::string("assigned_to").length(), "cc");
			bool ccSuccess = refresh(requestAddress);

			if (assignedSuccess && ccSuccess)
			{
				info.lastBugzillaRefresh = now;

				database.write_bugzilla_instance(info, *m_sender);
			}
		}
	}
}

void Bugzilla::load_instance(const BugzillaInstance& instance)
{
	m_bugzilla.emplace(instance.bugzillaName, instance);
}

void Bugzilla::next_instance_id(BugzillaInstanceID next)
{
	m_nextBugzillaID = next;
}
