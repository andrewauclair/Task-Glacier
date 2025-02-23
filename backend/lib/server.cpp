#include "server.hpp"
#include "clock.hpp"

#include <format>

Task::Task(std::string name, TaskID id, TaskID parentID, std::chrono::milliseconds createTime) : m_name(std::move(name)), m_taskID(id), m_parentID(parentID), m_createTime(createTime) {}

bool Task::operator==(const Task& task) const
{
	return m_taskID == task.m_taskID;
}

std::expected<TaskID, std::string> MicroTask::create_task(const std::string& name, TaskID parentID)
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

std::vector<MicroTask::FindTasksOnDay> MicroTask::find_tasks_on_day(int month, int year, int day)
{
	std::vector<FindTasksOnDay> tasks;

	auto range = range_for_date(month, year, day);

	for (auto&& task : m_tasks)
	{
		int index = 0;
		for (auto&& times : task.second.m_times)
		{
			if (times.start >= range.start && times.start <= range.end)
			{
				tasks.emplace_back(&task.second, DailyReport::TimePair{ task.first, index });
			}
			else if (times.stop >= range.start && times.stop <= range.end)
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
		task->m_times.emplace_back(m_clock->now());

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
		task->m_times.back().stop = m_clock->now();

		*m_output << "stop " << id._val << ' ' << m_clock->now().count() << std::endl;

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

		*m_output << "finish " << id._val << ' ' << m_clock->now().count() << std::endl;

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

		*m_output << "reparent " << id._val << ' ' << new_parent_id._val << std::endl;

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

		*m_output << "rename " << id._val << ' ' << '(' << name << ')' << std::endl;

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
		else if (line.starts_with("rename"))
		{
			auto values = split(line, ' ');
			TaskID id = TaskID(std::stoi(values[1]));
			auto first = line.find_first_of('(') + 1;
			std::string name = line.substr(first, line.size() - first - 1);

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
			auto values = split(line, ' ');

			m_bugzillaURL = values[1];
			m_bugzillaApiKey = values[2];
			std::getline(input, m_bugzillaUsername);
			
			std::string temp;
			std::getline(input, temp);
			
			m_bugzillaRootTaskID = TaskID(std::stoi(temp));

			std::getline(input, m_bugzillaGroupTasksBy);
			std::getline(input, temp);

			const int labelCount = std::stoi(temp);

			for (int i = 0; i < labelCount; i++)
			{
				std::string label;
				std::string field;
				std::getline(input, label);
				std::getline(input, field);

				m_bugzillaLabelToField[label] = field;
			}
		}
		else if (line.starts_with("bugzilla-refresh"))
		{
			auto values = split(line, ' ');

			m_lastBugzillaRefresh = std::chrono::milliseconds(std::stoll(values[1]));
		}
		else
		{
			throw std::runtime_error("Invalid command: " + line);
		}
	}
}
