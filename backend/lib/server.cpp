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
	task.indexInParent = find_tasks_with_parent(parentID).size();

	m_tasks.emplace(id, task);
	
	m_nextTaskID._val++;

	m_database->write_task(task, *m_sender);
	m_database->write_next_task_id(m_nextTaskID, *m_sender);

	return std::expected<TaskID, std::string>(id);
}

std::optional<std::string> MicroTask::configure_task_time_entry(TaskID taskID, std::span<const TimeEntry> timeEntry)
{
	auto* task = find_task(taskID);

	if (task)
	{
		task->timeEntry = std::vector<TimeEntry>(timeEntry.begin(), timeEntry.end());

		m_database->write_task(*task, *m_sender);
	}

	return std::nullopt;
}

Task* MicroTask::find_task(TaskID taskID)
{
	// only allow the unspecified task to be found while it is active
	if (m_activeTask == &m_unspecifiedTask && taskID == UNSPECIFIED_TASK)
	{
		return &m_unspecifiedTask;
	}

	auto result = m_tasks.find(taskID);

	return result != m_tasks.end() ? &result->second : nullptr;
}

std::vector<Task*> MicroTask::find_tasks_with_parent(TaskID parentID)
{
	std::vector<Task*> tasks;

	for (auto&& [taskID, task] : m_tasks)
	{
		if (task.parentID() == parentID)
		{
			tasks.push_back(&task);
		}
	}

	return tasks;
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
	return start_task(id, m_clock->now());
}

std::optional<std::string> MicroTask::start_task(TaskID id, std::chrono::milliseconds startTime)
{
	auto* task = find_task(id);

	if (id == UNSPECIFIED_TASK)
	{
		task = &m_unspecifiedTask;

		if (task->state == TaskState::ACTIVE)
		{
			return std::format("Unspecified task is already active.");
		}

		m_unspecifiedTask.m_times.clear();
	}

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
			m_activeTask->state = TaskState::PENDING;
			m_activeTask->m_times.back().stop = startTime;

			m_database->write_task(*m_activeTask, *m_sender);
		}

		task->state = TaskState::ACTIVE;
		TaskTimes& times = task->m_times.emplace_back(startTime);

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

		m_database->write_task(*task, *m_sender);

		return std::nullopt;
	}
	return std::format("Task with ID {} does not exist.", id);
}

std::optional<std::string> MicroTask::stop_task(TaskID id)
{
	auto* task = find_task(id);

	if (id == UNSPECIFIED_TASK)
	{
		task = &m_unspecifiedTask;
	}

	if (task && task->state == TaskState::ACTIVE)
	{
		m_activeTask = nullptr;

		task->state = TaskState::PENDING;

		task->m_times.back().stop = m_clock->now();

		m_database->write_task(*task, *m_sender);

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

		m_database->write_task(*task, *m_sender);

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
	if (id == new_parent_id)
	{
		return std::format("Cannot reparent Task with ID {} to itself", id);
	}

	auto* task = find_task(id);
	auto* parent_task = find_task(new_parent_id);

	if (task && parent_task)
	{
		// check if parent_task is a descendant of task
		TaskID parentID = parent_task->taskID();

		while (parentID != NO_PARENT)
		{
			auto* next_parent = find_task(parentID);
			parentID = next_parent->parentID();

			if (parentID == id)
			{
				return std::format("Cannot reparent Task with ID {} to Task with ID {}. Task with ID {} is a descendant of Task with ID {}.", id, new_parent_id, id, new_parent_id);
			}
		}
	}

	if (task && (parent_task || new_parent_id == NO_PARENT))
	{
		task->m_parentID = new_parent_id;

		m_database->write_task(*task, *m_sender);

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

		m_database->write_task(*task, *m_sender);

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
	if (task.taskID() == UNSPECIFIED_TASK)
	{
		m_unspecifiedTask = task;

		if (task.state == TaskState::ACTIVE)
		{
			m_activeTask = &m_unspecifiedTask;
		}

		return;
	}

	m_tasks.emplace(task.taskID(), task);

	if (task.state == TaskState::ACTIVE)
	{
		m_activeTask = find_task(task.taskID());
	}
}

void MicroTask::load_time_entry(const std::vector<TimeCategory>& timeCategories)
{
	m_timeCategories = timeCategories;
}
