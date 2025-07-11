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

class API;
class Database;

class Task
{
	friend class MicroTask;

private:
	TaskID m_taskID;
	TaskID m_parentID;

	std::chrono::milliseconds m_createTime;

public:
	bool serverControlled = false;
	bool locked = false;

	std::vector<TimeEntry> timeEntry;
	std::vector<TaskTimes> m_times;
	std::optional<std::chrono::milliseconds> m_finishTime;

	std::vector<std::string> labels;

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
	MicroTask(API& api, const Clock& clock, std::ostream& output, Database& database) : m_api(&api), m_clock(&clock), m_output(&output), m_database(&database) {}

	std::expected<TaskID, std::string> create_task(const std::string& name, TaskID parentID = NO_PARENT, bool serverControlled = false);

	std::optional<std::string> configure_task_time_entry(TaskID taskID, std::span<const TimeEntry> timeEntry);

	Task* active_task() const { return m_activeTask; }
	Task* find_task(TaskID id);
	Task* find_task_with_parent_and_name(const std::string& name, TaskID parentID);

	void find_bugzilla_helper_tasks(TaskID bugzillaParentTaskID, const std::vector<TaskID>& bugTasks, std::map<TaskID, TaskState>& helperTasks);

	bool task_has_children(TaskID id) const;
	bool task_has_active_bug_tasks(TaskID id, const std::vector<TaskID>& bugTasks) const;

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

	std::vector<TimeCategory>& timeCategories() { return m_timeCategories; }
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
	
public:
	TimeCategoryID m_nextTimeCategoryID = TimeCategoryID(1);
	TimeCodeID m_nextTimeCodeID = TimeCodeID(1);

private:
	std::unordered_map<TaskID, Task> m_tasks;
	Task* m_activeTask = nullptr;

	TaskID m_nextTaskID = TaskID(1);

	std::vector<TimeCategory> m_timeCategories;

	const Clock* m_clock;
	std::ostream* m_output;
	Database* m_database;

	API* m_api;
};

#endif
