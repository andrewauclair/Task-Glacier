#ifndef MICRO_TASK_PACKETS_HPP
#define MICRO_TASK_PACKETS_HPP

#include "lib.hpp"

#include <vector>
#include <variant>
#include <string>
#include <cstring>
#include <memory>

#include <strong_type/strong_type.hpp>

struct UnpackError {
	enum {
		NOT_ENOUGH_BYTES
	};
};

using RequestID = strong::type<std::int32_t, struct request_id_, strong::equality>;

struct MessageVisitor;

enum class PacketType : std::int32_t
{
	CREATE_TASK = 1,
	CREATE_LIST,
	CREATE_GROUP,

	MOVE_TASK,
	MOVE_LIST,
	MOVE_GROUP,

	START_TASK,
	STOP_TASK,
	FINISH_TASK,

	SUCCESS_RESPONSE,
	FAILURE_RESPONSE,

	REQUEST_CONFIGURATION,
	REQUEST_CONFIGURATION_COMPLETE,

	TASK_INFO,
	LIST_INFO,
	GROUP_INFO,
};

struct Message {
	virtual void visit(MessageVisitor& visitor) const = 0;
};

struct CreateTaskMessage : Message
{
	ListID listID;
	RequestID requestID;
	std::string name;

	CreateTaskMessage(ListID listID, RequestID requestID, std::string name) : listID(listID), requestID(requestID), name(std::move(name)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const CreateTaskMessage& other) const
	{
		return listID == other.listID && requestID == other.requestID && name == other.name;
	}

	std::vector<std::byte> pack() const;
	static std::expected<CreateTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const CreateTaskMessage& message)
	{
		out << "CreateTaskMessage { ListID: " << message.listID._val << ", RequestID: " << message.requestID._val << ", Name: \"" << message.name << "\" }";
		return out;
	}
};

struct CreateListMessage : Message
{
	GroupID groupID;
	RequestID requestID;
	std::string name;

	CreateListMessage(GroupID groupID, RequestID requestID, std::string name) : groupID(groupID), requestID(requestID), name(std::move(name)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const CreateListMessage& other) const
	{
		return groupID == other.groupID && requestID == other.requestID && name == other.name;
	}

	std::vector<std::byte> pack() const;
	static std::expected<CreateListMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const CreateListMessage& message)
	{
		out << "CreateListMessage { GroupID: " << message.groupID._val << ", RequestID: " << message.requestID._val << ", Name: \"" << message.name << "\" }";
		return out;
	}
};

struct CreateGroupMessage : Message
{
	GroupID groupID;
	RequestID requestID;
	std::string name;

	CreateGroupMessage(GroupID groupID, RequestID requestID, std::string name) : groupID(groupID), requestID(requestID), name(std::move(name)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const CreateGroupMessage& other) const
	{
		return groupID == other.groupID && requestID == other.requestID && name == other.name;
	}

	std::vector<std::byte> pack() const;
	static std::expected<CreateGroupMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const CreateGroupMessage& message)
	{
		out << "CreateGroupMessage { GroupID: " << message.groupID._val << ", RequestID: " << message.requestID._val << ", Name: \"" << message.name << "\" }";
		return out;
	}
};

struct SuccessResponse : Message
{
	RequestID requestID;

	SuccessResponse(RequestID requestID) : requestID(requestID) {}

	void visit(MessageVisitor& visitor) const override {};

	bool operator==(SuccessResponse other) const { return requestID == other.requestID; }

	std::vector<std::byte> pack() const;

	friend std::ostream& operator<<(std::ostream& out, const SuccessResponse& message)
	{
		out << "SuccessResponse { RequestID: " << message.requestID._val << " }";
		return out;
	}
};

struct FailureResponse : Message
{
	RequestID requestID;
	std::string message;

	FailureResponse(RequestID requestID, std::string message) : requestID(requestID), message(std::move(message)) {}

	void visit(MessageVisitor& visitor) const override {};

	bool operator==(const FailureResponse& other) const
	{
		return requestID == other.requestID && message == other.message;
	}

	std::vector<std::byte> pack() const;

	friend std::ostream& operator<<(std::ostream& out, const FailureResponse& message)
	{
		out << "FailureResponse { RequestID: " << message.requestID._val << ", message: \"" << message.message << "\" }";
		return out;
	}
};

struct EmptyMessage : Message
{
	PacketType packetType;

	EmptyMessage(PacketType type) : packetType(type) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const EmptyMessage& other) const
	{
		return packetType == other.packetType;
	}

	std::vector<std::byte> pack() const;
	static std::expected<EmptyMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const EmptyMessage& message)
	{
		out << "EmptyMessage { PacketType: " << static_cast<std::uint32_t>(message.packetType) << " }";
		return out;
	}
};

struct TaskInfoMessage : Message
{
	TaskID taskID;
	ListID listID;
	std::string name;

	TaskInfoMessage(TaskID taskID, ListID listID, std::string name) : taskID(taskID), listID(listID), name(std::move(name)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const TaskInfoMessage& other) const
	{
		return taskID == other.taskID && listID == other.listID && name == other.name;
	}

	std::vector<std::byte> pack() const;
	static std::expected<TaskInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const TaskInfoMessage& message)
	{
		out << "TaskInfoMessage { TaskID: " << message.taskID._val << ", ListID: " << message.listID._val << ", Name : \"" << message.name << "\" }";
		return out;
	}
};

struct ListInfoMessage : Message
{
	GroupID groupID;
	ListID listID;
	std::string name;

	ListInfoMessage(GroupID groupID, ListID listID, std::string name) : groupID(groupID), listID(listID), name(std::move(name)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const ListInfoMessage& other) const
	{
		return groupID == other.groupID && listID == other.listID && name == other.name;
	}

	std::vector<std::byte> pack() const;
	static std::expected<ListInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const ListInfoMessage& message)
	{
		out << "ListInfoMessage { GroupID: " << message.groupID._val << ", ListID: " << message.listID._val << ", Name : \"" << message.name << "\" }";
		return out;
	}
};

struct GroupInfoMessage : Message
{
	GroupID groupID;
	std::string name;

	GroupInfoMessage(GroupID groupID, std::string name) : groupID(groupID), name(std::move(name)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const GroupInfoMessage& other) const
	{
		return groupID == other.groupID && name == other.name;
	}

	std::vector<std::byte> pack() const;
	static std::expected<GroupInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const GroupInfoMessage& message)
	{
		out << "GroupInfoMessage { GroupID: " << message.groupID._val << ", Name: \"" << message.name << "\" }";
		return out;
	}
};

struct MessageVisitor {
	virtual void visit(const CreateTaskMessage&) = 0;
	virtual void visit(const CreateListMessage&) = 0;
	virtual void visit(const CreateGroupMessage&) = 0;
	virtual void visit(const SuccessResponse&) {}
	virtual void visit(const FailureResponse&) {}
	virtual void visit(const EmptyMessage&) = 0;
	virtual void visit(const TaskInfoMessage&) {};
	virtual void visit(const ListInfoMessage&) {};
	virtual void visit(const GroupInfoMessage&) {};
};

class PacketBuilder
{
private:
	std::vector<std::byte> m_bytes;

public:

	std::vector<std::byte> build()
	{
		std::int32_t length = m_bytes.size() + sizeof(std::int32_t);

		auto* ptr = reinterpret_cast<std::byte*>(&length);

		// inserting in this order will do the byte swapping for us atm
		for (int i = 0; i < sizeof(length); i++, ptr++)
		{
			m_bytes.insert(m_bytes.begin(), *ptr);
		}
		return m_bytes;
	}

	template<typename T>
	void add(T value) = delete;

	template<typename T>
		requires std::integral<T> || std::floating_point<T>
	void add(T value)
	{
		T swapped = std::byteswap(value);
		auto* current_byte = reinterpret_cast<std::byte*>(&swapped);

		for (int i = 0; i < sizeof(T); i++, current_byte++)
		{
			m_bytes.push_back(*current_byte);
		}
	}

	template<typename T>
		requires strong::is_strong_type<T>::value
	void add(T value)
	{
		add(value._val);
	}

	void add_string(std::string_view str)
	{
		std::int16_t size = str.size();

		add(size);

		for (auto ch : str)
		{
			m_bytes.push_back(static_cast<std::byte>(ch));
		}
	}
};

class PacketParser
{
private:
	std::span<const std::byte> data;
	std::span<const std::byte>::iterator position;

public:
	PacketParser(std::span<const std::byte> data) : data(data), position(data.begin())
	{
	}

	template<typename T>
	std::expected<T, UnpackError> parse_value()
	{
		T value;

		if (std::distance(position, data.end()) < sizeof(T))
		{
			return UnpackError::NOT_ENOUGH_BYTES;
		}

		position += sizeof(T);

		return value;
	}
};

struct ParseResult
{
	std::optional<std::unique_ptr<Message>> packet;
	std::int32_t bytes_read = 0;
};

inline ParseResult parse_packet(std::span<const std::byte> bytes)
{
	ParseResult result;

	if (bytes.size() > 4)
	{
		std::int32_t raw_length;
		std::memcpy(&raw_length, bytes.data(), sizeof(raw_length));
		raw_length = std::byteswap(raw_length);

		result.bytes_read += sizeof(raw_length);

		// read out the packet type
		std::int32_t raw_type;
		std::memcpy(&raw_type, bytes.data() + result.bytes_read, sizeof(PacketType));
		const PacketType type = static_cast<PacketType>(std::byteswap(raw_type));

		result.bytes_read += sizeof(PacketType);

		switch (type)
		{
			using enum PacketType;

		case CREATE_TASK:
			result.packet = std::make_unique<CreateTaskMessage>(CreateTaskMessage::unpack(bytes.subspan(8)).value());
			result.bytes_read = raw_length;

			break;
		case CREATE_LIST:
		{
			result.packet = std::make_unique<CreateListMessage>(CreateListMessage::unpack(bytes.subspan(8)).value());
			result.bytes_read = raw_length;

			break;
		}
		case CREATE_GROUP:
		{
			result.packet = std::make_unique<CreateGroupMessage>(CreateGroupMessage::unpack(bytes.subspan(8)).value());
			result.bytes_read = raw_length;

			break;
		}
		case REQUEST_CONFIGURATION:
		case REQUEST_CONFIGURATION_COMPLETE:
		{
			result.packet = std::make_unique<EmptyMessage>(EmptyMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;

			break;
		}
		default:
			break;
		}
	}
	return result;
}

#endif
