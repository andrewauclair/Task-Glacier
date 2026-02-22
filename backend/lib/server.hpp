#ifndef MICRO_TASK_LIB_HPP
#define MICRO_TASK_LIB_HPP

#include "clock.hpp"

#include "packets/task_id.hpp"
#include "packets/time_entry.hpp"
#include "packets/task_times.hpp"
#include "packets/task_state.hpp"
#include "packets/task_info.hpp"
#include "packets/daily_report.hpp"
#include "packets/time_category.hpp"
#include "packets/basic.hpp"

#include "packet_sender.hpp"

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
#include <set>
#include <map>

#include <strong_type/strong_type.hpp>

class API;
struct Database;

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

	std::int32_t indexInParent = 0;

	std::vector<TimeEntry> timeEntry;
	std::vector<TaskTimes> m_times;
	std::optional<std::chrono::milliseconds> m_finishTime;

	std::vector<std::string> labels;

	Task(std::string name, TaskID id, TaskID parentID, std::chrono::milliseconds createTime);

	constexpr auto operator<=>(const Task&) const = default;

	TaskID taskID() const { return m_taskID; }
	TaskID parentID() const { return m_parentID; }

	std::chrono::milliseconds createTime() const { return m_createTime; }

	std::string m_name;
	TaskState state = TaskState::PENDING;

	friend std::ostream& operator<<(std::ostream& out, const Task& task)
	{
		out << "Task { ";
		out << "id: " << task.m_taskID._val << ", parent: " << task.m_parentID._val;
		out << ", create: " << task.m_createTime << ", serverControlled: " << task.serverControlled;
		out << ", locked: " << task.locked;
		out << ", state: " << static_cast<std::int32_t>(task.state);
		if (task.m_finishTime)
		{
			out << ", finish: " << task.m_finishTime.value();
		}
		else
		{
			out << ", finish: empty";
		}
		for (auto&& t : task.timeEntry) out << t;
		for (auto&& t : task.m_times) out << t;
		out << "}";
		return out;
	}
};

class MicroTask
{
public:
	MicroTask(API& api, const Clock& clock, Database& database, PacketSender& sender)
	: m_clock(&clock),
	m_database(&database),
	m_sender(&sender),
	m_api(&api),
	m_unspecifiedTask("unspecified", UNSPECIFIED_TASK, NO_PARENT, std::chrono::milliseconds(0))
	{}

	std::expected<TaskID, std::string> create_task(const std::string& name, TaskID parentID = NO_PARENT, bool serverControlled = false);

	std::optional<std::string> configure_task_time_entry(TaskID taskID, std::span<const TimeEntry> timeEntry);

	Task* active_task() const { return m_activeTask; }
	Task* find_task(TaskID id);
	std::vector<Task*> find_tasks_with_parent(TaskID parentID);
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
	std::optional<std::string> start_task(TaskID id, std::chrono::milliseconds startTime);
	std::optional<std::string> stop_task(TaskID id);
	std::optional<std::string> finish_task(TaskID id);

	std::optional<std::string> reparent_task(TaskID id, TaskID new_parent_id);
	std::optional<std::string> rename_task(TaskID id, std::string_view name);

	void fill_session_time_entry(const Task& task, TaskTimes& times);

	std::expected<TaskState, std::string> task_state(TaskID id);

	void load_task(const Task& task);
	void load_time_entry(const std::vector<TimeCategory>& timeCategories);

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

	void send_task_info(const Task& task, bool newTask)
	{
		auto info = std::make_unique<TaskInfoMessage>(task.taskID(), task.parentID(), task.m_name);

		info->state = task.state;
		info->createTime = task.createTime();
		info->finishTime = task.m_finishTime;
		info->newTask = newTask;
		info->indexInParent = task.indexInParent;
		info->serverControlled = task.serverControlled;
		info->locked = task.locked;
		info->times = task.m_times;
		info->timeEntry = task.timeEntry;
		info->labels = task.labels;

		m_sender->send(std::move(info));
	}

	void send_all_tasks()
	{
		m_sender->send(std::make_unique<BasicMessage>(PacketType::BULK_TASK_INFO_START));

		std::vector<TaskID> parents;

		parents.push_back(NO_PARENT);

		while (!parents.empty())
		{
			std::vector<TaskID> next;

			for (TaskID parent : parents)
			{
				if (parent != NO_PARENT)
				{
					send_task_info(m_tasks.at(parent), false);
				}

				auto all_tasks = find_tasks_with_parent(parent);

				std::sort(all_tasks.begin(), all_tasks.end(),
					[](Task* a, Task* b)
					{
						return a->taskID() < b->taskID();
					}
				);

				for (auto&& task : all_tasks)
				{
					// skip the unspecified task
					if (task->taskID() == UNSPECIFIED_TASK)
					{
						continue;
					}
					next.push_back(task->taskID());
				}
			}
			parents = next;
		}

		if (m_activeTask == &m_unspecifiedTask)
		{
			m_sender->send(std::make_unique<BasicMessage>(PacketType::UNSPECIFIED_TASK_ACTIVE));
		}

		m_sender->send(std::make_unique<BasicMessage>(PacketType::BULK_TASK_INFO_FINISH));
	}

	TimeCategories& timeCategories() { return m_timeCategories; }
	
	bool is_bulk_update() const { return m_bulk_update; }
	void add_update_task(TaskID id) { m_changedTasksBulkUpdate.insert(id); }
	void start_bulk_update() { m_bulk_update = true; }
	void finish_bulk_update()
	{
		m_bulk_update = false;

		m_sender->send(std::make_unique<BasicMessage>(PacketType::BULK_TASK_INFO_START));

		for (TaskID task : m_changedTasksBulkUpdate)
		{
			send_task_info(m_tasks.at(task), false);
		}
		m_changedTasksBulkUpdate.clear();

		m_sender->send(std::make_unique<BasicMessage>(PacketType::BULK_TASK_INFO_FINISH));
	}

public:
	TimeCategoryID m_nextTimeCategoryID = TimeCategoryID(1);
	TimeCodeID m_nextTimeCodeID = TimeCodeID(1);
	TaskID m_nextTaskID = TaskID(1);

private:
	std::unordered_map<TaskID, Task> m_tasks;
	Task m_unspecifiedTask;
	Task* m_activeTask = nullptr;

	bool m_bulk_update = false;
	std::set<TaskID> m_changedTasksBulkUpdate;

	TimeCategories m_timeCategories;

	const Clock* m_clock;
	Database* m_database;
	PacketSender* m_sender;

	API* m_api;
};

#endif
