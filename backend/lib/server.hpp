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

class Task
{
	friend class MicroTask;

private:
	TaskID m_taskID;
	TaskID m_parentID;

	std::chrono::milliseconds m_createTime;

public:
	std::vector<TaskTimes> m_times;
	std::optional<std::chrono::milliseconds> m_finishTime;

	Task(std::string name, TaskID id, TaskID parentID, std::chrono::milliseconds createTime);

	bool operator==(const Task& task) const;

	TaskID taskID() const { return m_taskID; }
	TaskID parentID() const { return m_parentID; }

	std::chrono::milliseconds createTime() const { return m_createTime; }

	std::string m_name;
	TaskState state = TaskState::INACTIVE;
};

class MicroTask
{
public:
	MicroTask(const Clock& clock, std::ostream& output) : m_clock(&clock), m_output(&output) {}

	std::expected<TaskID, std::string> create_task(const std::string& name, TaskID parentID = NO_PARENT);

	Task* active_task() const { return m_activeTask; }
	Task* find_task(TaskID id);

	struct FindTasksOnDay
	{
		Task* task;
		DailyReport::TimePair time;
	};
	std::vector<FindTasksOnDay> find_tasks_on_day(int month, int year, int day);

	std::optional<std::string> start_task(TaskID id);
	std::optional<std::string> stop_task(TaskID id);
	std::optional<std::string> finish_task(TaskID id);

	std::optional<std::string> reparent_task(TaskID id, TaskID new_parent_id);
	std::optional<std::string> rename_task(TaskID id, std::string_view name);
	
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

	const std::vector<TimeCategory> timeCategories() const { return m_timeCategories; }
	std::optional<std::string> add_time_category(std::string_view name)
	{
		// error if time category with name already exists

		return std::nullopt;
	}
	
	std::optional<std::string> add_time_code(TimeCodeID id, std::string_view category, std::string_view name)
	{
		// error if time category doesn't exist

		// error if time category already has time code with name

		return std::nullopt;
	}
	
	std::optional<std::string> add_time_code(std::string_view category, std::string_view name)
	{
		auto id = m_nextTimeCodeID;
		m_nextTimeCodeID++;

		// error if time category doesn't exist

		// error if time category already has time code with name

		return std::nullopt;
	}
	
	std::string m_bugzillaURL;
	std::string m_bugzillaApiKey;
	std::string m_bugzillaUsername;
	TaskID m_bugzillaRootTaskID = NO_PARENT;
	std::string m_bugzillaGroupTasksBy;
	std::map<std::string, std::string> m_bugzillaLabelToField;
	std::optional<std::chrono::milliseconds> m_lastBugzillaRefresh;

	std::map<int, TaskID> m_bugzillaTasks;
	std::map<std::string, TaskID> m_bugzillaGroupBy;

private:
	std::unordered_map<TaskID, Task> m_tasks;
	Task* m_activeTask = nullptr;

	TaskID m_nextTaskID = TaskID(1);

	std::vector<TimeCategory> m_timeCategories;
	TimeCodeID m_nextTimeCodeID = TimeCodeID(1);

	const Clock* m_clock;
	std::ostream* m_output;

	
};

#endif
