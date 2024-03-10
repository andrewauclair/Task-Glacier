#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <expected>
#include <cstdint>
#include <variant>
#include <span>
#include <bit>

using TaskID = std::int32_t;
using ListID = std::int32_t;
using GroupID = std::int32_t;

inline constexpr GroupID ROOT_GROUP_ID = 0;

enum class TaskState
{
	INACTIVE,ACTIVE,FINISHED
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

	TaskID m_nextTaskID = 1;
	ListID m_nextListID = 1;
	GroupID m_nextGroupID = 1;
};

enum class PacketType : std::int32_t
{
	CREATE_TASK,
	CREATE_LIST,
	CREATE_GROUP,

	MOVE_TASK,
	MOVE_LIST,
	MOVE_GROUP,

	START_TASK,
	STOP_TASK,
	FINISH_TASK,
};

struct CreateListPacket
{
	std::string name;
	GroupID groupID;
};

struct CreateGroupPacket
{
	std::string name;
	GroupID groupID;
};

using PacketTypes = std::variant<CreateListPacket, CreateGroupPacket, std::monostate>;

struct ParseResult
{
	std::optional<PacketTypes> packet;
	std::int32_t bytes_read = 0;
};

inline ParseResult parse_packet(std::span<const std::byte> bytes)
{
	ParseResult result;

	if (bytes.size() > 4)
	{
		// read out the packet type
		std::int32_t raw_type;
		std::memcpy(&raw_type, bytes.data(), sizeof(PacketType));
		const PacketType type = static_cast<PacketType>(std::byteswap(raw_type));

		result.bytes_read += sizeof(PacketType);

		switch (type)
		{
			using enum PacketType;

		case CREATE_TASK:
			break;
		case CREATE_LIST:
		{
			CreateListPacket create_list;

			std::int32_t raw_group_id;
			std::memcpy(&raw_group_id, bytes.data() + result.bytes_read, sizeof(std::int32_t));

			result.bytes_read += sizeof(std::int32_t);

			create_list.groupID = std::byteswap(raw_group_id);

			std::int16_t raw_name_length;
			std::memcpy(&raw_name_length, bytes.data() + result.bytes_read, sizeof(std::int16_t));

			result.bytes_read += sizeof(std::int16_t);

			auto length = std::byteswap(raw_name_length);
			create_list.name.resize(length);
			std::memcpy(create_list.name.data(), bytes.data() + result.bytes_read, length);

			result.bytes_read += length;

			result.packet = create_list;

			break;
		}
		default:
			break;
		}
	}
	return result;
}
