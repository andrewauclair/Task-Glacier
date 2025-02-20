#include "server.hpp"

#include <format>

Task::Task(std::string name, TaskID id, TaskID parentID, std::chrono::milliseconds createTime) : m_name(std::move(name)), m_taskID(id), m_parentID(parentID), m_createTime(createTime) {}

bool Task::operator==(const Task& task) const
{
	return m_taskID == task.m_taskID;
}

std::expected<TaskID, std::string> MicroTask::create_task(const std::string& name, TaskID parentID)
{
	if (parentID._val != 0 && !m_tasks.contains(parentID))
	{
		return std::unexpected(std::format("Task with ID {} does not exist.", parentID));
	}

	auto id = m_nextTaskID;

	m_tasks.emplace(id, Task(name, id, parentID, m_clock->now()));

	m_nextTaskID._val++;

	*m_output << "create " << id._val << ' ' << parentID._val << ' ' << m_clock->now().count() << " (" << name << ")" << std::endl;

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
		if (m_activeTask)
		{
			m_activeTask->state = TaskState::INACTIVE;
		}
		task->state = TaskState::ACTIVE;

		m_activeTask = task;

		*m_output << "start " << id._val << ' ' << m_clock->now().count() << std::endl;

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

		*m_output << "stop " << id._val << ' ' << m_clock->now().count() << std::endl;

		return std::nullopt;
	}
	return std::format("");
}

std::optional<std::string> MicroTask::finish_task(TaskID id)
{
	auto* task = find_task(id);

	if (task && task->state != TaskState::FINISHED)
	{
		if (task == m_activeTask)
		{
			m_activeTask = nullptr;
		}

		task->state = TaskState::FINISHED;

		*m_output << "finish " << id._val << ' ' << m_clock->now().count() << std::endl;

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

namespace
{
	std::vector<std::string> split(const std::string& s, char delim) {
		std::vector<std::string> result;
		std::stringstream ss(s);
		std::string item;

		while (getline(ss, item, delim)) {
			result.push_back(item);
		}

		return result;
	}
}

void MicroTask::load_from_file(std::istream& input)
{
	std::string line;

	Task* activeTask = nullptr;

	while (std::getline(input, line))
	{
		if (line.starts_with("create"))
		{
			auto values = split(line, ' ');
			TaskID id = TaskID(std::stoi(values[1]));
			TaskID parentID = TaskID(std::stoi(values[2]));
			std::chrono::milliseconds createTime = std::chrono::milliseconds(std::stoll(values[3]));
			auto first = line.find_first_of('(') + 1;
			std::string name = line.substr(first, line.size() - first - 1);

			m_tasks.emplace(id, Task(name, id, parentID, createTime));

			// attempt to track the next task ID
			// TODO test this
			m_nextTaskID._val = id._val + 1;
		}
		else if (line.starts_with("start"))
		{
			auto values = split(line, ' ');
			TaskID id = TaskID(std::stoi(values[1]));
			std::chrono::milliseconds startTime = std::chrono::milliseconds(std::stoll(values[2]));

			auto* task = find_task(id);

			if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));

			if (activeTask)
			{
				activeTask->state = TaskState::INACTIVE;
				activeTask->m_times.back().stop = startTime;
			}
			activeTask = task;

			task->state = TaskState::ACTIVE;
			task->m_times.emplace_back(startTime);
		}
		else if (line.starts_with("stop"))
		{
			auto values = split(line, ' ');
			TaskID id = TaskID(std::stoi(values[1]));
			std::chrono::milliseconds stopTime = std::chrono::milliseconds(std::stoll(values[2]));

			auto* task = find_task(id);

			if (!task) throw std::runtime_error("Task not found: " + std::to_string(id._val));
			if (task->m_times.empty()) throw std::runtime_error("Cannot stop task, never been started: " + std::to_string(id._val));

			activeTask = nullptr;
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
				activeTask = nullptr;
			}

			task->state = TaskState::FINISHED;
			task->m_finishTime = finishTime;
		}
		else
		{
			throw std::runtime_error("Invalid command: " + line);
		}
	}
}
