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
#include <chrono>
#include <map>
#include <array>
#include <cassert>

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

enum class TaskState : std::int32_t
{
	INACTIVE,
	ACTIVE,
	FINISHED
};

inline constexpr TaskID NO_PARENT = TaskID(0);

using RequestID = strong::type<std::int32_t, struct request_id_, strong::equality, strong::incrementable>;

struct MessageVisitor;

enum class PacketType : std::int32_t
{
	VERSION_REQUEST = 1,
	VERSION = 2,

	CREATE_TASK = 3,
	MOVE_TASK = 4,
	START_TASK = 5,
	STOP_TASK = 6,
	FINISH_TASK = 7,
	UPDATE_TASK = 15, // TODO redo numbers when we're done
	REQUEST_TASK = 22,

	SUCCESS_RESPONSE = 8,
	FAILURE_RESPONSE = 9,

	REQUEST_CONFIGURATION = 10,
	REQUEST_CONFIGURATION_COMPLETE = 11,

	TASK_INFO = 12,

	// configure and refresh bugzilla (refresh is manual by the user on the UI)
	// the server will create and update tasks based on bugzilla changes and send TASK_INFO messages
	// both of these messages have responses of SUCCESS_RESPONSE or FAILURE_RESPONSE
	BUGZILLA_INFO = 13,
	BUGZILLA_REFRESH = 14,

	// report for the day with various information, displayed on a dialog on the UI
	DAILY_REPORT = 16,
	REQUEST_DAILY_REPORT = 17,

	// report for the time tracking for the week, displayed on a dialog on the UI
	WEEKLY_REPORT = 18,
	REQUEST_WEEKLY_REPORT = 19,

	// request the server to search for tasks matching the provided information
	SEARCH_REQUEST = 20,
	// return the results to the UI. This is a list of task IDs matching the search request
	SEARCH_RESULTS = 21,

	// configure the backup, including the IP and port of the backup service, how often to perform a backup, and how many backups to keep
	BACKUP_CONFIGURATION = 23,
	// backup has been successfully performed
	BACKUP_PERFORMED = 24,
	// backup has failed. error message and last successful backup time are provided
	BACKUP_FAILED = 25,
};

struct Message
{
public:
	Message(PacketType packetType) : m_packetType(packetType) {}

	PacketType packetType() const { return m_packetType; }

	virtual bool operator==(const Message& message) const = 0;

	virtual std::vector<std::byte> pack() const = 0;

	virtual std::ostream& print(std::ostream& out) const = 0
	{
		out << "packetType: " << static_cast<std::int32_t>(m_packetType);
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const Message& message)
	{
		return message.print(out);
	}

private:
	PacketType m_packetType;
};

struct RequestMessage : Message
{
	RequestID requestID;

	RequestMessage(PacketType packetType, RequestID requestID) : Message(packetType), requestID(requestID) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const RequestMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const RequestMessage& message) const
	{
		return packetType() == message.packetType() && requestID == message.requestID;
	}

	std::ostream& print(std::ostream& out) const override
	{
		Message::print(out);
		out << ", requestID: " << requestID._val;
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const RequestMessage& message)
	{
		return message.print(out);
	}
};

struct CreateTaskMessage : RequestMessage
{
	TaskID parentID;
	std::string name;

	CreateTaskMessage(TaskID parentID, RequestID requestID, std::string name) : RequestMessage(PacketType::CREATE_TASK, requestID), parentID(parentID), name(std::move(name)) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const CreateTaskMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}
	
	bool operator==(const CreateTaskMessage& message) const
	{
		return parentID == message.parentID && requestID == message.requestID && name == message.name;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<CreateTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "CreateTaskMessage { ";
		RequestMessage::print(out);
		out << ", parentID: " << parentID._val << ", name: \"" << name << "\" }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const CreateTaskMessage& message)
	{
		return message.print(out);
	}
};

struct TaskMessage : RequestMessage
{
	TaskID taskID;

	TaskMessage(PacketType type, RequestID requestID, TaskID taskID) : RequestMessage(type, requestID), taskID(taskID)
	{
		assert(type == PacketType::START_TASK || type == PacketType::STOP_TASK || type == PacketType::FINISH_TASK || type == PacketType::REQUEST_TASK);
	}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const TaskMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const TaskMessage& message) const
	{
		return packetType() == message.packetType() && taskID == message.taskID && requestID == message.requestID;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TaskMessage { ";
		RequestMessage::print(out);
		out << ", taskID: " << taskID._val << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const TaskMessage& message)
	{
		message.print(out);
		return out;
	}
};

struct SuccessResponse : Message
{
	RequestID requestID;

	SuccessResponse(RequestID requestID) : Message(PacketType::SUCCESS_RESPONSE), requestID(requestID) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const SuccessResponse*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const SuccessResponse& message) const
	{
		return requestID == message.requestID;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<SuccessResponse, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "SuccessResponse { ";
		Message::print(out);
		out << ", requestID: " << requestID._val << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const SuccessResponse& message)
	{
		message.print(out);
		return out;
	}
};

struct FailureResponse : Message
{
	RequestID requestID;
	std::string message;

	FailureResponse(RequestID requestID, std::string message) : Message(PacketType::FAILURE_RESPONSE), requestID(requestID), message(std::move(message)) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const FailureResponse*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const FailureResponse& message) const
	{
		return requestID == message.requestID && this->message == message.message;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<FailureResponse, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "FailureResponse { ";
		Message::print(out);
		out << ", requestID: " << requestID._val << ", message: \"" << message << "\" }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const FailureResponse& message)
	{
		message.print(out);
		return out;
	}
};

struct BasicMessage : Message
{
	BasicMessage(PacketType type) : Message(type) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const BasicMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const BasicMessage& message) const
	{
		return packetType() == message.packetType();
	}

	std::vector<std::byte> pack() const override;
	static std::expected<BasicMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "BasicMessage { ";
		Message::print(out);
		out << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const BasicMessage& message)
	{
		message.print(out);
		return out;
	}
};

struct TaskTimes
{
	std::chrono::milliseconds start = std::chrono::milliseconds(0);
	std::optional<std::chrono::milliseconds> stop;
};

struct TaskInfoMessage : Message
{
	TaskID taskID;
	TaskID parentID;
	TaskState state = TaskState::INACTIVE;
	bool newTask = false;

	std::string name;

	std::chrono::milliseconds createTime = std::chrono::milliseconds(0);
	std::vector<TaskTimes> times;
	std::optional<std::chrono::milliseconds> finishTime;

	TaskInfoMessage(TaskID taskID, TaskID parentID, std::string name, std::chrono::milliseconds createTime = std::chrono::milliseconds(0)) : Message(PacketType::TASK_INFO), taskID(taskID), parentID(parentID), name(std::move(name)), createTime(createTime) {}
	
	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const TaskInfoMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const TaskInfoMessage& message) const
	{
		if (times.size() != message.times.size())
		{
			return false;
		}
		
		for (std::size_t i = 0; i < times.size(); i++)
		{
			if (times[i].start != message.times[i].start || times[i].stop != message.times[i].stop)
			{
				return false;
			}
		}

		return taskID == message.taskID && parentID == message.parentID && state == message.state && newTask == message.newTask && name == message.name && createTime == message.createTime && finishTime == message.finishTime;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TaskInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TaskInfoMessage { taskID: " << taskID._val << ", parentID: " << parentID._val << ", state: " << static_cast<std::int32_t>(state) << ", newTask: " << newTask << ", name: \"" << name << "\", createTime: " << createTime.count() << ", finishTime: " << (finishTime.has_value() ? std::to_string(finishTime.value().count()) : "nullopt") << ", times: [";
		for (auto&& time : times)
		{
			out << "{ start: " << time.start.count() << ", stop: " << (time.stop.has_value() ? std::to_string(time.stop.value().count()) : "nullopt") << " }, ";
		}
		out << "]";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const TaskInfoMessage& message)
	{
		out << "TaskInfoMessage { taskID: " << message.taskID._val << ", parentID: " << message.parentID._val << ", state: " << static_cast<std::int32_t>(message.state) << ", newTask: " << message.newTask << ", name: \"" << message.name << "\", createTime: " << message.createTime.count() << ", finishTime: " << (message.finishTime.has_value() ? std::to_string(message.finishTime.value().count()) : "nullopt") << ", times: [";
		for (auto&& time : message.times)
		{
			out << "{ start: " << time.start.count() << ", stop: " << (time.stop.has_value() ? std::to_string(time.stop.value().count()) : "nullopt") << " }, ";
		}
		out << "]";
		return out;
	}
};

struct BugzillaInfoMessage : Message
{
	std::string URL;
	std::string apiKey;

	BugzillaInfoMessage(std::string URL, std::string apiKey) : Message(PacketType::BUGZILLA_INFO), URL(std::move(URL)), apiKey(std::move(apiKey)) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const BugzillaInfoMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const BugzillaInfoMessage& message) const
	{
		return URL == message.URL && apiKey == message.apiKey;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<BugzillaInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "BugzillaInfoMessage { URL: \"" << URL << "\", apiKey: \"" << apiKey << "\" }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const BugzillaInfoMessage& message)
	{
		out << "BugzillaInfoMessage { URL: \"" << message.URL << "\", apiKey: \"" << message.apiKey << "\" }";
		return out;
	}
};

struct DailyReport
{
	// reports for days that have no task start/stops is invalid (the UI will say there's no data or something for that day)
	bool isValidReport = false;

	// start time for the day
	std::chrono::milliseconds startTime;

	// estimated end time for the day (assuming 8 hours)
	bool estimatedEndTime = false;
	std::chrono::milliseconds endTime;

	// list of task ids and the start/stop index
	struct TimePair {
		TaskID taskID; std::int32_t startStopIndex;

		constexpr auto operator<=>(const TimePair&) const = default;
	};

	std::vector<TimePair> times;

	// total time for the day
	std::chrono::milliseconds totalTime;

	// total time per time category for the day
	std::map<std::string, std::chrono::milliseconds> timePerCategory;

	constexpr auto operator<=>(const DailyReport&) const = default;

	friend std::ostream& operator<<(std::ostream& out, const DailyReport& report)
	{
		return out;
	}
};

struct DailyReportMessage : Message
{
	DailyReport report;

	DailyReportMessage() : Message(PacketType::DAILY_REPORT) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const DailyReportMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const DailyReportMessage& message) const
	{
		return report == message.report;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<DailyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "DailyReportMessage { " << report << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const DailyReportMessage& message)
	{
		out << "DailyReportMessage { " << message.report << " }";
		return out;
	}
};

struct WeeklyReportMessage : Message
{
	std::array<DailyReport, 7> dailyReports;

	std::chrono::milliseconds totalTime;
	std::map<std::string, std::chrono::milliseconds> timePerCategory;

	// su, mo, tu, we, th, fr, sa
	// time spent on each time category per day
	// list of task ids and the start/stop index
	// total time per day
	// total time for week
	// total time per time category per day
	// total time per time category for week
};

struct SearchRequestMessage : Message
{
	// search text
	// type: task name, label, time, state (mainly for finished), time category
	std::string searchText;

	// ideally this would work kind of like the IntelliJ search anywhere, but with some special stuff for time
	// or maybe certain text is detected as time like "Jan 29" or "January 29" or "1/29"?
};

struct SearchResultMessage : Message
{
	std::vector<TaskID> taskIDs;
};

// time categories
// these will be tracked internally by ID so that we don't have to pass strings everywhere
// and the user can freely rename them and have all uses update
// the time categories can be removed after adding, but only if they have not been used
// the user can optionally reassign any times to a new time category continue removing
//
// time categories can be archived when in use. this will remove them from being an option
// for creating new tasks, but will allow them to still exist on old tasks. Some option might
// be needed for existing tasks that are using the time category. Presumably, if you are archiving
// a time category, it's no longer in use. so in this case, you should probably change any
// active tasks to another time category.

//struct MessageVisitor {
//	virtual void visit(const CreateTaskMessage&) = 0;
//	virtual void visit(const TaskMessage&) {}
//	virtual void visit(const SuccessResponse&) {}
//	virtual void visit(const FailureResponse&) {}
//	virtual void visit(const BasicMessage&) = 0;
//	virtual void visit(const TaskInfoMessage&) {}
//	virtual void visit(const BugzillaInfoMessage&) {}
//};

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
		requires (std::integral<T> || std::floating_point<T>) && (!std::same_as<T, bool>)
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
		requires std::same_as<T, bool>
	void add(T value)
	{
		add(static_cast<std::int8_t>(value));
	}

	template<typename T>
		requires strong::is_strong_type<T>::value
	void add(T value)
	{
		add(value._val);
	}

	template<typename T>
		requires std::same_as<T, std::chrono::milliseconds>
	void add(T value)
	{
		add(static_cast<std::int64_t>(value.count()));
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
		requires (std::integral<T> || std::floating_point<T>) && (!std::same_as<T, bool>)
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
		requires std::same_as<T, bool>
	std::expected<T, UnpackError> parse_next()
	{
		auto result = parse_next<std::int8_t>();

		if (result)
		{
			return result.value() != 0;
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

	template<typename T>
		requires std::same_as<T, std::chrono::milliseconds>
	std::expected<T, UnpackError> parse_next()
	{
		auto result = parse_next<std::int64_t>();

		if (result)
		{
			return std::chrono::milliseconds(result.value());
		}
		return std::unexpected(result.error());
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
		case START_TASK:
		case STOP_TASK:
		case FINISH_TASK:
		case REQUEST_TASK:
			result.packet = std::make_unique<TaskMessage>(TaskMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case SUCCESS_RESPONSE:
			result.packet = std::make_unique<SuccessResponse>(SuccessResponse::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case FAILURE_RESPONSE:
			result.packet = std::make_unique<FailureResponse>(FailureResponse::unpack(bytes.subspan(4)).value());
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
