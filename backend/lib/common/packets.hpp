#ifndef MICRO_TASK_PACKETS_HPP
#define MICRO_TASK_PACKETS_HPP

//#include "server.hpp"

#include <vector>
#include <variant>
#include <string>
#include <cstring>
#include <memory>
#include <expected>
#include <span>
#include <optional>

#include <strong_type/strong_type.hpp>

enum class UnpackError {
	NOT_ENOUGH_BYTES
};

using TaskID = strong::type<std::int32_t, struct task_id_, strong::equality, strong::hashable, strong::ordered>;

template <>
struct std::formatter<TaskID> : std::formatter<std::int32_t> {
	auto format(TaskID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};

inline constexpr TaskID NO_PARENT = TaskID(0);

using RequestID = strong::type<std::int32_t, struct request_id_, strong::equality, strong::incrementable>;

struct MessageVisitor;

enum class PacketType : std::int32_t
{
	VERSION_REQUEST = 1,
	VERSION,

	CREATE_TASK,
	MOVE_TASK,
	START_TASK,
	STOP_TASK,
	FINISH_TASK,

	SUCCESS_RESPONSE,
	FAILURE_RESPONSE,

	REQUEST_CONFIGURATION,
	REQUEST_CONFIGURATION_COMPLETE,

	TASK_INFO,

	BUGZILLA_INFO,
	BUGZILLA_REFRESH
};

struct Message
{
public:
	Message(PacketType packetType) : m_packetType(packetType) {}

	PacketType packetType() const { return m_packetType; }

	virtual void visit(MessageVisitor& visitor) const = 0;
	
	virtual std::vector<std::byte> pack() const = 0;

private:
	PacketType m_packetType;
};

struct CreateTaskMessage : Message
{
	TaskID parentID;
	RequestID requestID;
	std::string name;

	CreateTaskMessage(TaskID parentID, RequestID requestID, std::string name) : Message(PacketType::CREATE_TASK), parentID(parentID), requestID(requestID), name(std::move(name)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const CreateTaskMessage& other) const
	{
		return parentID == other.parentID && requestID == other.requestID && name == other.name;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<CreateTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const CreateTaskMessage& message)
	{
		out << "CreateTaskMessage { parentID: " << message.parentID._val << ", RequestID: " << message.requestID._val << ", Name: \"" << message.name << "\" }";
		return out;
	}
};

struct TaskMessage : Message
{
	TaskID taskID;

	TaskMessage(PacketType type, TaskID taskID) : Message(type), taskID(taskID) {}
};

struct StartTaskMessage : TaskMessage
{
	RequestID requestID;

	StartTaskMessage(TaskID taskID, RequestID requestID) : TaskMessage(PacketType::START_TASK, taskID), requestID(requestID) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const StartTaskMessage& other) const
	{
		return taskID == other.taskID && requestID == other.requestID;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<StartTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const StartTaskMessage& message)
	{
		out << "StartTaskMessage { taskID: " << message.taskID._val << "\" }";
		return out;
	}
};

struct StopTaskMessage : TaskMessage
{
	RequestID requestID;

	StopTaskMessage(TaskID taskID, RequestID requestID) : TaskMessage(PacketType::STOP_TASK, taskID), requestID(requestID) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const StopTaskMessage& other) const
	{
		return taskID == other.taskID && requestID == other.requestID;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<StopTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const StopTaskMessage& message)
	{
		out << "StopTaskMessage { taskID: " << message.taskID._val << "\" }";
		return out;
	}
};

struct FinishTaskMessage : TaskMessage
{
	RequestID requestID;

	FinishTaskMessage(TaskID taskID, RequestID requestID) : TaskMessage(PacketType::FINISH_TASK, taskID), requestID(requestID) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const FinishTaskMessage& other) const
	{
		return taskID == other.taskID && requestID == other.requestID;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<FinishTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const FinishTaskMessage& message)
	{
		out << "FinishTaskMessage { taskID: " << message.taskID._val << "\" }";
		return out;
	}
};

struct SuccessResponse : Message
{
	RequestID requestID;

	SuccessResponse(RequestID requestID) : Message(PacketType::SUCCESS_RESPONSE), requestID(requestID) {}

	void visit(MessageVisitor& visitor) const override {};

	bool operator==(SuccessResponse other) const { return requestID == other.requestID; }

	std::vector<std::byte> pack() const override;

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

	FailureResponse(RequestID requestID, std::string message) : Message(PacketType::FAILURE_RESPONSE), requestID(requestID), message(std::move(message)) {}

	void visit(MessageVisitor& visitor) const override {};

	bool operator==(const FailureResponse& other) const
	{
		return requestID == other.requestID && message == other.message;
	}

	std::vector<std::byte> pack() const override;

	friend std::ostream& operator<<(std::ostream& out, const FailureResponse& message)
	{
		out << "FailureResponse { RequestID: " << message.requestID._val << ", message: \"" << message.message << "\" }";
		return out;
	}
};

struct BasicMessage : Message
{
	PacketType packetType;

	BasicMessage(PacketType type) : Message(packetType), packetType(type) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const BasicMessage& other) const
	{
		return packetType == other.packetType;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<BasicMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const BasicMessage& message)
	{
		out << "BasicMessage { PacketType: " << static_cast<std::uint32_t>(message.packetType) << " }";
		return out;
	}
};

struct TaskInfoMessage : Message
{
	TaskID taskID;
	TaskID parentID;
	std::string name;

	TaskInfoMessage(TaskID taskID, TaskID parentID, std::string name) : Message(PacketType::TASK_INFO), taskID(taskID), parentID(parentID), name(std::move(name)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const TaskInfoMessage& other) const
	{
		return taskID == other.taskID && parentID == other.parentID && name == other.name;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TaskInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const TaskInfoMessage& message)
	{
		out << "TaskInfoMessage { TaskID: " << message.taskID._val << ", ParentID: " << message.parentID._val << ", Name : \"" << message.name << "\" }";
		return out;
	}
};

struct BugzillaInfoMessage : Message
{
	std::string URL;
	std::string apiKey;

	BugzillaInfoMessage(std::string URL, std::string apiKey) : Message(PacketType::BUGZILLA_INFO), URL(std::move(URL)), apiKey(std::move(apiKey)) {}

	void visit(MessageVisitor& visitor) const override;

	bool operator==(const BugzillaInfoMessage& other) const
	{
		return URL == other.URL && apiKey == other.apiKey;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<BugzillaInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	friend std::ostream& operator<<(std::ostream& out, const BugzillaInfoMessage& message)
	{
		out << "BugzillaInfoMessage { URL: \"" << message.URL << "\", apiKey: \"" << message.apiKey << "\" }";
		return out;
	}
};

struct MessageVisitor {
	virtual void visit(const CreateTaskMessage&) = 0;
	virtual void visit(const StartTaskMessage&) {}
	virtual void visit(const StopTaskMessage&) {}
	virtual void visit(const FinishTaskMessage&) {}
	virtual void visit(const SuccessResponse&) {}
	virtual void visit(const FailureResponse&) {}
	virtual void visit(const BasicMessage&) = 0;
	virtual void visit(const TaskInfoMessage&) {}
	virtual void visit(const BugzillaInfoMessage&) {}
};

class PacketBuilder
{
private:
	std::vector<std::byte> m_bytes;

public:

	std::vector<std::byte> build() const
	{
		std::vector<std::byte> bytes = m_bytes;

		std::int32_t length = bytes.size() + sizeof(std::int32_t);

		auto* ptr = reinterpret_cast<std::byte*>(&length);

		// inserting in this order will do the byte swapping for us atm
		for (int i = 0; i < sizeof(length); i++, ptr++)
		{
			bytes.insert(bytes.begin(), *ptr);
		}
		return bytes;
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
		requires std::is_enum_v<T>
	void add(T value)
	{
		add(static_cast<std::underlying_type_t<T>>(value));
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
	std::size_t position;

public:
	PacketParser(std::span<const std::byte> data) : data(data), position(0)
	{
	}

	template<typename T>
	std::expected<T, UnpackError> parse_next() = delete;

	template<typename T>
		requires std::integral<T> || std::floating_point<T>
	std::expected<T, UnpackError> parse_next()
	{
		if (std::distance(data.begin() + position, data.end()) < sizeof(T))
		{
			return std::unexpected(UnpackError::NOT_ENOUGH_BYTES);
		}

		T value;
		std::memcpy(&value, data.data() + position, sizeof(T));
		value = std::byteswap(value);

		position += sizeof(T);

		return value;
	}

	template<typename T>
		requires std::is_enum_v<T>
	std::expected<T, UnpackError> parse_next()
	{
		auto result = parse_next<std::underlying_type_t<T>>();

		if (result)
		{
			return T(result.value());
		}
		return std::unexpected(result.error());
	}

	template<typename T>
		requires strong::is_strong_type<T>::value
	std::expected<T, UnpackError> parse_next()
	{
		auto result = parse_next<strong::underlying_type_t<T>>();

		if (result)
		{
			return T(result.value());
		}
		return std::unexpected(result.error());
	}

	template<typename T>
		requires std::same_as<T, std::string>
	std::expected<T, UnpackError> parse_next()
	{
		auto length = parse_next<std::int16_t>();

		if (!length)
		{
			return std::unexpected(length.error());
		}

		if (std::distance(data.begin() + position, data.end()) < length.value())
		{
			return std::unexpected(UnpackError::NOT_ENOUGH_BYTES);
		}

		std::string name;
		name.resize(length.value());
		std::memcpy(name.data(), data.data() + position, length.value());
		position += length.value();

		return name;
	}
};

struct ParseResult
{
	std::unique_ptr<Message> packet;
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
			result.packet = std::make_unique<CreateTaskMessage>(CreateTaskMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;

			break;
		case REQUEST_CONFIGURATION:
		case REQUEST_CONFIGURATION_COMPLETE:
		case BUGZILLA_REFRESH:
		{
			result.packet = std::make_unique<BasicMessage>(BasicMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;

			break;
		}
		case BUGZILLA_INFO:
		{
			result.packet = std::make_unique<BugzillaInfoMessage>(BugzillaInfoMessage::unpack(bytes.subspan(4)).value());
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
