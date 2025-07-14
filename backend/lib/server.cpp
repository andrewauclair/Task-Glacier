#include "server.hpp"
#include "clock.hpp"
#include "api.hpp"

#include <format>
#include <iostream>

Task::Task(std::string name, TaskID id, TaskID parentID, std::chrono::milliseconds createTime) : m_name(std::move(name)), m_taskID(id), m_parentID(parentID), m_createTime(createTime) {}

std::expected<TaskID, std::string> MicroTask::create_task(const std::string& name, TaskID parentID, bool serverControlled)
{
	auto* parent_task = find_task(parentID);

	if (parentID._val != 0 && !parent_task)
	{
		return std::unexpected(std::format("Task with ID {} does not exist.", parentID));
	}
	else if (parent_task && parent_task->state == TaskState::FINISHED)
	{
		return std::unexpected(std::format("Cannot add sub-task. Task with ID {} is finished.", parentID));
	}

	auto id = m_nextTaskID;

	Task task = Task(name, id, parentID, m_clock->now());
	task.serverControlled = serverControlled;

	m_tasks.emplace(id, task);
	
	m_nextTaskID._val++;

	// TODO we need to remove this again. Adding it back so that the tests pass the same as they used to
	m_clock->now();

	//*m_output << "create " << id._val << ' ' << parentID._val << ' ' << m_clock->now().count() << ' ' << task.serverControlled << ' ' << persist_string(name) << std::endl;

	m_database->write_task(task);
	m_database->write_next_task_id(m_nextTaskID);

	return std::expected<TaskID, std::string>(id);
}

std::optional<std::string> MicroTask::configure_task_time_entry(TaskID taskID, std::span<const TimeEntry> timeEntry)
{
	auto* task = find_task(taskID);

	if (task)
	{
		task->timeEntry = std::vector<TimeEntry>(timeEntry.begin(), timeEntry.end());

		/**m_output << "task-time-codes " << task->m_taskID._val << ' ';
		
		if (task->timeEntry.empty())
		{
			*m_output << "0 0 ";
		}
		else
		{
			for (auto&& time : task->timeEntry)
			{
				*m_output << time.categoryID._val << ' ' << time.codeID._val << ' ';
			}
		}
		*m_output << std::endl;*/

		m_database->write_task(*task);
	}

	return std::nullopt;
}

Task* MicroTask::find_task(TaskID taskID)
{
	auto result = m_tasks.find(taskID);

	return result != m_tasks.end() ? &result->second : nullptr;
}

Task* MicroTask::find_task_with_parent_and_name(const std::string& name, TaskID parentID)
{
	for (auto&& [taskID, task] : m_tasks)
	{
		if (task.parentID() == parentID && task.m_name == name)
		{
			return &task;
		}
	}
	return nullptr;
}

void MicroTask::find_bugzilla_helper_tasks(TaskID bugzillaParentTaskID, const std::vector<TaskID>& bugTasks, std::map<TaskID, TaskState>& helperTasks)
{
	for (auto&& [taskID, task] : m_tasks)
	{
		if (task.parentID() == bugzillaParentTaskID && std::find(bugTasks.begin(), bugTasks.end(), task.taskID()) == bugTasks.end())
		{
			helperTasks[task.taskID()] = task.state;

			find_bugzilla_helper_tasks(task.taskID(), bugTasks, helperTasks);
		}
	}
}

bool MicroTask::task_has_children(TaskID id) const
{
	for (auto&& [taskID, task] : m_tasks)
	{
		if (task.parentID() == id)
		{
			return true;
		}
	}
	return false;
}

bool MicroTask::task_has_active_bug_tasks(TaskID id, const std::vector<TaskID>& bugTasks) const
{
	for (auto&& [taskID, task] : m_tasks)
	{
		const bool isBug = std::find(bugTasks.begin(), bugTasks.end(), taskID) != bugTasks.end();
		
		// return true if:
		//  - parent matches and this is a bug
		//  - or, task_has_bug_tasks is true for any children
		if (task.parentID() == id && isBug && task.state != TaskState::FINISHED)
		{
			return true;
		}

		if (task.parentID() != id)
		{
			continue;
		}

		if (task_has_active_bug_tasks(taskID, bugTasks))
		{
			return true;
		}
	}
	return false;
}

std::vector<MicroTask::FindTasksOnDay> MicroTask::find_tasks_on_day(int month, int year, int day)
{
	std::vector<FindTasksOnDay> tasks;

	auto range = range_for_date(month, year, day);

	for (auto&& task : m_tasks)
	{
		int index = 0;
		for (auto&& times : task.second.m_times)
		{
			if (times.start >= range.start && times.start < range.end)
			{
				tasks.emplace_back(&task.second, DailyReport::TimePair{ task.first, index });
			}
			else if (times.stop >= range.start && times.stop < range.end)
			{
				tasks.emplace_back(&task.second, DailyReport::TimePair{ task.first, index });
			}
			index++;
		}
	}

	return tasks;
}

std::optional<std::string> MicroTask::start_task(TaskID id)
{
	auto* task = find_task(id);

	if (task)
	{
		if (task->state == TaskState::ACTIVE)
		{
			return std::format("Task with ID {} is already active.", id);
		}
		else if (task->state == TaskState::FINISHED)
		{
			return std::format("Task with ID {} is finished.", id);
		}

		if (m_activeTask)
		{
			m_activeTask->state = TaskState::INACTIVE;
			m_activeTask->m_times.back().stop = m_clock->now();
		}

		task->state = TaskState::ACTIVE;
		TaskTimes& times = task->m_times.emplace_back(m_clock->now());

		for (const TimeCategory& category : m_timeCategories)
		{
			const auto findCategory = [category](const Task* task)
				{
					auto result = std::find_if(task->timeEntry.begin(), task->timeEntry.end(), [&](const TimeEntry& entry) { return entry.categoryID == category.id; });

					return result;
				};

			auto taskResult = findCategory(task);

			// check if the task has this category, if it does not, move onto the parent
			// if not found in the parent, fill with unknown
			if (taskResult != task->timeEntry.end())
			{
				times.timeEntry.push_back(*taskResult);
			}
			else
			{
				// didn't find the category in the task, check the parents
				auto* parent = find_task(task->parentID());

				while (parent && findCategory(parent) == parent->timeEntry.end())
				{
					parent = find_task(parent->parentID());
				}

				if (parent)
				{
					auto result = findCategory(parent);

					// if we found a parent with the category, use its time code
					if (result != parent->timeEntry.end())
					{
						times.timeEntry.push_back(*result);
					}
					else
					{
						// if we didn't find a parent with the category, use unknown
						times.timeEntry.emplace_back(category.id, TimeCodeID(0));
					}
				}
				else
				{
					// if we didn't find a parent with the category, use unknown
					times.timeEntry.emplace_back(category.id, TimeCodeID(0));
				}
			}
		}

		m_activeTask = task;

		// TODO we need to remove this again. Adding it back so that the tests pass the same as they used to
		m_clock->now();

		/**m_output << "start " << id._val << ' ' << m_clock->now().count() << ' ';

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
		*m_output << std::endl;*/

		m_database->write_task(*task);

		return std::nullopt;
	}
	return std::format("Task with ID {} does not exist.", id);
}

std::optional<std::string> MicroTask::stop_task(TaskID id)
{
	auto* task = find_task(id);

	if (task && task->state == TaskState::ACTIVE)
	{
		m_activeTask = nullptr;

		task->state = TaskState::INACTIVE;
		task->m_times.back().stop = m_clock->now();

		// TODO we need to remove this again. Adding it back so that the tests pass the same as they used to
		m_clock->now();

		//*m_output << "stop " << id._val << ' ' << m_clock->now().count() << std::endl;

		m_database->write_task(*task);

		return std::nullopt;
	}
	if (!task)
	{
		return std::format("Task with ID {} does not exist.", id);
	}
	return std::format("Task with ID {} is not active.", id);
}

std::optional<std::string> MicroTask::finish_task(TaskID id)
{
	auto* task = find_task(id);

	if (task && task->state != TaskState::FINISHED)
	{
		auto finish_time = m_clock->now();

		if (task == m_activeTask)
		{
			task->m_times.back().stop = finish_time;

			m_activeTask = nullptr;
		}

		task->m_finishTime = finish_time;

		task->state = TaskState::FINISHED;

		// TODO we need to remove this again. Adding it back so that the tests pass the same as they used to
		m_clock->now();

		//*m_output << "finish " << id._val << ' ' << m_clock->now().count() << std::endl;

		m_database->write_task(*task);

		return std::nullopt;
	}
	if (task && task->state == TaskState::FINISHED)
	{
		return std::format("Task with ID {} is already finished.", id);
	}
	return std::format("Task with ID {} does not exist.", id);
}

std::optional<std::string> MicroTask::reparent_task(TaskID id, TaskID new_parent_id)
{
	auto* task = find_task(id);
	auto* parent_task = find_task(new_parent_id);

	if (task && (parent_task || new_parent_id == NO_PARENT))
	{
		task->m_parentID = new_parent_id;

		//*m_output << "reparent " << id._val << ' ' << new_parent_id._val << std::endl;

		m_database->write_task(*task);

		return std::nullopt;
	}
	if (!parent_task)
	{
		return std::format("Task with ID {} does not exist.", new_parent_id);
	}
	return std::format("Task with ID {} does not exist.", id);
}

std::optional<std::string> MicroTask::rename_task(TaskID id, std::string_view name)
{
	auto* task = find_task(id);

	if (task)
	{
		task->m_name = name;

		//*m_output << "rename " << id._val << ' ' << persist_string(task->m_name) << std::endl;

		m_database->write_task(*task);

		return std::nullopt;
	}
	return std::format("Task with ID {} does not exist.", id);
}

std::expected<TaskState, std::string> MicroTask::task_state(TaskID id)
{
	const auto* task = find_task(id);

	if (task)
	{
		return task->state;
	}
	return std::unexpected("");
}

void MicroTask::load_task(const Task& task)
{
	m_tasks.emplace(task.taskID(), task);
}

void MicroTask::load_time_entry(const std::vector<TimeCategory>& timeCategories)
{
	m_timeCategories = timeCategories;
}

void MicroTask::load_from_file(std::istream& input)
{
	const auto string_parser = [](std::string_view& str) -> std::string
		{
			// (4 test)
			// (7 foo bar)
			
			// first, chop off opening parenthese
			str = str.substr(str.find_first_of('(') + 1);

			auto values = split(std::string(str), ' ');

			std::int32_t count = std::stoi(values[0]);

			str = str.substr(values[0].size() + 1);

			auto result = std::string(str.substr(0, count));

			str = str.substr(result.size() + 1);

			return result;
		};

	std::string line;
	std::int32_t lineCount = 0;

	try
	{
		while (std::getline(input, line))
		{
			lineCount++;

			if (line.starts_with("create"))
			{
				auto values = split(line, ' ');
				TaskID id = TaskID(std::stoi(values[1]));
				TaskID parentID = TaskID(std::stoi(values[2]));
				std::chrono::milliseconds createTime = std::chrono::milliseconds(std::stoll(values[3]));
				bool serverControlled = std::stoi(values[4]);

				std::string_view view = line;
				auto task = Task(string_parser(view), id, parentID, createTime);
				task.serverControlled = serverControlled;

				m_tasks.emplace(id, std::move(task));

				// attempt to track the next task ID
				// TODO test this
				m_nextTaskID._val = id._val + 1;
			}
			else if (line.starts_with("task-time-codes"))
			{
				auto values = split(line, ' ');

				TaskID id = TaskID(std::stoi(values[1]));

				auto* task = find_task(id);

				if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));

				task->timeEntry.clear();

				for (int i = 2; i < values.size() - 1; i += 2)
				{
					auto category = TimeCategoryID(std::stoi(values[i]));
					auto code = TimeCodeID(std::stoi(values[i + 1]));

					if (category._val != 0 && code._val != 0)
					{
						task->timeEntry.push_back(TimeEntry{ category, code });
					}
				}
			}
			else if (line.starts_with("start"))
			{
				auto values = split(line, ' ');
				TaskID id = TaskID(std::stoi(values[1]));
				std::chrono::milliseconds startTime = std::chrono::milliseconds(std::stoll(values[2]));

				auto* task = find_task(id);

				if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));

				if (m_activeTask)
				{
					m_activeTask->state = TaskState::INACTIVE;
					m_activeTask->m_times.back().stop = startTime;
				}
				m_activeTask = task;

				task->state = TaskState::ACTIVE;
				auto& times = task->m_times.emplace_back(startTime);

				for (int i = 3; i < values.size() - 1; i += 2)
				{
					auto category = TimeCategoryID(std::stoi(values[i]));
					auto code = TimeCodeID(std::stoi(values[i + 1]));

					if (category._val != 0 && code._val != 0)
					{
						times.timeEntry.push_back(TimeEntry{ category, code });
					}
				}
			}
			else if (line.starts_with("stop"))
			{
				auto values = split(line, ' ');
				TaskID id = TaskID(std::stoi(values[1]));
				std::chrono::milliseconds stopTime = std::chrono::milliseconds(std::stoll(values[2]));

				auto* task = find_task(id);

				if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));
				if (task->m_times.empty()) throw std::runtime_error("Cannot stop task, never been started: " + std::to_string(id._val));

				m_activeTask = nullptr;
				task->state = TaskState::INACTIVE;
				task->m_times.back().stop = stopTime;
			}
			else if (line.starts_with("finish"))
			{
				auto values = split(line, ' ');
				TaskID id = TaskID(std::stoi(values[1]));
				std::chrono::milliseconds finishTime = std::chrono::milliseconds(std::stoll(values[2]));

				auto* task = find_task(id);

				if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));

				if (task->state == TaskState::ACTIVE)
				{
					task->m_times.back().stop = finishTime;
					m_activeTask = nullptr;
				}

				task->state = TaskState::FINISHED;
				task->m_finishTime = finishTime;
			}
			else if (line.starts_with("unfinish"))
			{
				auto values = split(line, ' ');
				TaskID id = TaskID(std::stoi(values[1]));

				auto* task = find_task(id);

				if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));

				task->state = TaskState::INACTIVE;
				task->m_finishTime = std::nullopt;
			}
			else if (line.starts_with("rename"))
			{
				auto values = split(line, ' ');
				TaskID id = TaskID(std::stoi(values[1]));
				auto first = line.find_first_of('(') + 1;
				std::string_view view = line;
				std::string name = string_parser(view);//line.substr(first, line.size() - first - 1);

				auto* task = find_task(id);

				if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));

				task->m_name = name;
			}
			else if (line.starts_with("reparent"))
			{
				auto values = split(line, ' ');
				TaskID id = TaskID(std::stoi(values[1]));
				TaskID parent_id = TaskID(std::stoi(values[2]));

				auto* task = find_task(id);

				if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));

				task->m_parentID = parent_id;
			}
			else if (line.starts_with("bugzilla-config"))
			{
				m_api->m_bugzilla.load_config(line, input);
			}
			else if (line.starts_with("bugzilla-refresh"))
			{
				std::string tasks;
				std::getline(input, tasks);

				m_api->m_bugzilla.load_refresh(line, tasks);
			}
			else if (line.starts_with("time-category"))
			{
				auto values = split(line, ' ');
				auto idnum = std::stoi(values[2]);

				if (line.starts_with("time-category add"))
				{
					TimeCategoryID id = TimeCategoryID(std::stoi(values[2]));
					TimeCategory* timeCategory = nullptr;

					std::string_view view = line;
					std::string name = string_parser(view);
					std::string label = string_parser(view);
					
					if (id._val == 0)
					{
						id = m_nextTimeCategoryID;
						m_nextTimeCategoryID++;

						TimeCategory category{ id, name, label };

						timeCategory = &m_timeCategories.emplace_back(id, name, label);
					}
					else if (id._val >= m_nextTimeCategoryID._val)
					{
						m_nextTimeCategoryID = id;
						m_nextTimeCategoryID++;
					}

					if (!timeCategory)
					{
						for (auto&& it : m_timeCategories)
						{
							if (it.id == id)
							{
								timeCategory = &it;
								break;
							}
						}
					}

					values = split(std::string(view).substr(1), ' ');

					int count = std::stoi(values[0]);

					view = view.substr(values[0].size() + 1);

					for (int i = 0; i < count; i++)
					{
						values = split(std::string(view).substr(1), ' ');

						TimeCodeID code = TimeCodeID(std::stoi(values[0]));
						TimeCode* timeCode = nullptr;

						std::string codeName = string_parser(view);
						
						if (code._val == 0)
						{
							code = m_nextTimeCodeID;
							m_nextTimeCodeID++;

							timeCode = &timeCategory->codes.emplace_back(code, codeName);
						}
						else if (code._val >= m_nextTimeCodeID._val)
						{
							m_nextTimeCodeID = code;
							m_nextTimeCodeID++;
						}

						if (!timeCode)
						{
							for (auto&& it : timeCategory->codes)
							{
								if (it.id == code)
								{
									timeCode = &it;
									break;
								}
							}
						}

						timeCode->name = codeName;
					}
				}
				else if (line.starts_with("time-category update"))
				{
					TimeCategoryID id = TimeCategoryID(idnum);
					std::string_view view = line;
					std::string name = string_parser(view);
					std::string label = string_parser(view);
					
					auto result = std::find_if(m_timeCategories.begin(), m_timeCategories.end(), [&](auto&& it) { return it.id == id; });

					if (result != m_timeCategories.end())
					{
						result->name = name;
						result->label = label;

						result->codes.clear();

						values = split(std::string(view).substr(1), ' ');

						int count = std::stoi(values[0]);
						
						view = view.substr(values[0].size() + 1);

						for (int i = 0; i < count; i++)
						{
							values = split(std::string(view).substr(1), ' ');

							TimeCodeID code = TimeCodeID(std::stoi(values[0]));
							std::string codeName = string_parser(view);
							
							result->codes.emplace_back(code, codeName);
						}
					}
				}
				else if (line.starts_with("time-category remove-code"))
				{
					TimeCategoryID id = TimeCategoryID(idnum);

					auto result = std::find_if(m_timeCategories.begin(), m_timeCategories.end(), [&](auto&& it) { return it.id == id; });

					if (result != m_timeCategories.end())
					{
						for (int i = 4; i < values.size(); i++)
						{
							if (values[i] != "}")
							{
								TimeCodeID code = TimeCodeID(std::stoi(values[i]));
								std::erase_if(result->codes, [&](auto&& it) { return it.id == code; });
							}
						}
					}
				}
				else if (line.starts_with("time-category remove-category"))
				{
					TimeCategoryID id = TimeCategoryID(idnum);

					std::erase_if(m_timeCategories, [&](auto&& it) { return it.id == id; });
				}
			}
			else
			{
				throw std::runtime_error("Invalid command: " + line);
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cout << "[IN] Failed to load file, line: " << lineCount << " : " << line << std::endl;
	}
}
