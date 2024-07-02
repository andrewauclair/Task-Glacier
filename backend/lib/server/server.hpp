#ifndef MICRO_TASK_LIB_HPP
#define MICRO_TASK_LIB_HPP

#include "packets.hpp"

#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <expected>
#include <cstdint>
#include <variant>
#include <span>
#include <bit>
#include <algorithm>

#include <strong_type/strong_type.hpp>

inline constexpr TaskID NO_PARENT = TaskID(0);

enum class TaskState
{
	INACTIVE,
	ACTIVE,
	FINISHED
};

class Task
{
private:
	TaskID m_taskID;
	TaskID m_parentID;

public:
	Task(std::string name, TaskID id, TaskID parentID);

	TaskID taskID() const { return m_taskID; }
	TaskID parentID() const { return m_parentID; }

	std::string m_name;
	TaskState state = TaskState::INACTIVE;
};

class MicroTask
{
public:
	std::expected<TaskID, std::string> create_task(const std::string& name, TaskID parentID = NO_PARENT);

	Task* find_task(TaskID id);

	std::optional<std::string> start_task(TaskID id);
	std::optional<std::string> stop_task(TaskID id);
	std::optional<std::string> finish_task(TaskID id);
	
	std::expected<TaskState, std::string> task_state(TaskID id);

	template<typename Func>
	void for_each_task_sorted(Func&& func)
	{
		std::vector<TaskID> keys;
		keys.reserve(m_tasks.size());

		for (auto&& task : m_tasks)
		{
			keys.push_back(task.first);
		}

		std::sort(keys.begin(), keys.end());

		for (TaskID key : keys)
		{
			func(m_tasks.at(key));
		}
	}

private:
	std::unordered_map<TaskID, Task> m_tasks;

	TaskID m_nextTaskID = TaskID(1);
};

#endif
