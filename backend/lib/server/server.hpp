#ifndef MICRO_TASK_LIB_HPP
#define MICRO_TASK_LIB_HPP

#include "packets.hpp"
#include "clock.hpp"

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

enum class TaskState
{
	INACTIVE,
	ACTIVE,
	FINISHED
};

class Task
{
	friend class MicroTask;

private:
	TaskID m_taskID;
	TaskID m_parentID;

	std::chrono::milliseconds m_createTime;
	std::vector<TaskTimes> m_times;
	std::optional<std::chrono::milliseconds> m_finishTime;

public:
	Task(std::string name, TaskID id, TaskID parentID, std::chrono::milliseconds createTime);

	bool operator==(const Task& task) const;

	TaskID taskID() const { return m_taskID; }
	TaskID parentID() const { return m_parentID; }

	std::chrono::milliseconds createTime() const { return m_createTime; }
	std::span<const TaskTimes> times() const { return m_times; }
	std::optional<std::chrono::milliseconds> finishTime() const { return m_finishTime; }

	std::string m_name;
	TaskState state = TaskState::INACTIVE;
};

class MicroTask
{
public:
	MicroTask(const Clock& clock, std::ostream& output) : m_clock(&clock), m_output(&output) {}

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

	void load_from_file(std::istream& input);

private:
	std::unordered_map<TaskID, Task> m_tasks;

	TaskID m_nextTaskID = TaskID(1);

	const Clock* m_clock;
	std::ostream* m_output;
};

#endif
