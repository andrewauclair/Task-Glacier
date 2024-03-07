#include <string>
#include <optional>
#include <unordered_map>
#include <expected>

using TaskID = std::int32_t;
using ListID = std::int32_t;
using GroupID = std::int32_t;

class Task
{
public:
	std::string name;
};

class List
{
	std::vector<Task> m_tasks;
};

class Group
{
public:
	std::vector<Group> m_groups;
	std::vector<List> m_lists;
};

class MicroTask
{
public:
	std::expected<TaskID, std::string> create_task(const std::string& name, ListID listID);
	std::expected<ListID, std::string> create_list(const std::string& name, GroupID groupID);
	std::expected<GroupID, std::string> create_group(const std::string& name, GroupID groupID);

	std::optional<Task> find_task(TaskID id);

private:
	List* find_list(ListID listID);
	Group* find_group(GroupID groupID);
private:
	Group m_root;

};
