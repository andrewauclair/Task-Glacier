#include "server.hpp"

#include <format>

Task::Task(std::string name, TaskID id, TaskID parentID) : m_name(std::move(name)), m_taskID(id), m_parentID(parentID) {}

std::expected<TaskID, std::string> MicroTask::create_task(const std::string& name, TaskID parentID)
{
	if (parentID._val != 0 && !m_tasks.contains(parentID))
	{
		return std::unexpected(std::format("Task with ID {} does not exist.", parentID));
	}

	auto id = m_nextTaskID;

	m_tasks.emplace(id, Task(name, id, parentID));

	m_nextTaskID._val++;

	return std::expected<TaskID, std::string>(id);
}

Task* MicroTask::find_task(TaskID taskID)
{
	auto result = m_tasks.find(taskID);

	return result != m_tasks.end() ? &result->second : nullptr;
}

std::optional<std::string> MicroTask::start_task(TaskID id)
{
	auto* task = find_task(id);

	if (task)
	{
		task->state = TaskState::ACTIVE;

		return std::nullopt;
	}
	return std::format("Task with ID {} does not exist.", id);
}

std::optional<std::string> MicroTask::stop_task(TaskID id)
{
	auto* task = find_task(id);

	if (task && task->state == TaskState::ACTIVE)
	{
		task->state = TaskState::INACTIVE;

		return std::nullopt;
	}
	return std::format("");
}

std::optional<std::string> MicroTask::finish_task(TaskID id)
{
	auto* task = find_task(id);

	if (task && task->state == TaskState::ACTIVE)
	{
		task->state = TaskState::FINISHED;

		return std::nullopt;
	}
	return std::format("");
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
