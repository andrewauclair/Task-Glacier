#ifndef MICRO_TASK_LIB_HPP
#define MICRO_TASK_LIB_HPP

#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <expected>
#include <cstdint>
#include <variant>
#include <span>
#include <bit>

#include <strong_type/strong_type.hpp>

using TaskID = strong::type<std::int32_t, struct task_id_, strong::equality>;
using ListID = strong::type<std::int32_t, struct list_id_, strong::equality>;
using GroupID = strong::type<std::int32_t, struct group_id_, strong::equality>;

inline constexpr GroupID ROOT_GROUP_ID = GroupID(0);

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

public:
	Task(std::string name, TaskID id);

	TaskID taskID() const { return m_taskID; }

	std::string m_name;
	TaskState state = TaskState::INACTIVE;
};

class List
{
public:
	explicit List(std::string name, ListID id);

	std::string_view name() const { return m_name; }
	ListID listID() const { return m_listID; }

	std::vector<Task> m_tasks;

private:
	std::string m_name;
	ListID m_listID;
};

class Group
{
public:
	explicit Group(std::string name, GroupID id);

	std::string_view name() const { return m_name; }
	GroupID groupID() const { return m_groupID; }

	std::vector<Group> m_groups;
	std::vector<List> m_lists;

private:
	std::string m_name;
	GroupID m_groupID;
};

class MicroTask
{
public:
	std::expected<TaskID, std::string> create_task(const std::string& name, ListID listID);
	std::expected<ListID, std::string> create_list(const std::string& name, GroupID groupID);
	std::expected<GroupID, std::string> create_group(const std::string& name, GroupID groupID);

	std::optional<std::string> move_list(ListID listID, GroupID targetGroupID);
	std::optional<std::string> move_group(GroupID groupID, GroupID targetGroupID);

	Task* find_task(TaskID id);

	std::optional<std::string> start_task(TaskID id);
	
	std::expected<TaskState, std::string> task_state(TaskID id);

private:
	List* find_list_by_id(ListID listID);
	Group* find_group_by_id(GroupID groupID);

	List* find_list_by_name(std::string_view name, Group* group = nullptr);
	Group* find_group_by_name(std::string_view name, Group* group = nullptr);

private:
	Group m_root = Group("", ROOT_GROUP_ID);

	TaskID m_nextTaskID = TaskID(1);
	ListID m_nextListID = ListID(1);
	GroupID m_nextGroupID = GroupID(1);
};

#endif
