#ifndef MICRO_TASK_PACKETS_HPP
#define MICRO_TASK_PACKETS_HPP

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

using RequestID = strong::type<std::int32_t, struct request_id_, strong::equality, strong::incrementable, strong::ordered, strong::partially_ordered>;

template <>
struct std::formatter<RequestID> : std::formatter<std::int32_t> {
	auto format(RequestID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};

using TimeCodeID = strong::type<std::int32_t, struct time_code_id_, strong::equality, strong::incrementable, strong::ordered, strong::partially_ordered>;

template <>
struct std::formatter<TimeCodeID> : std::formatter<std::int32_t> {
	auto format(TimeCodeID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};

using TimeCategoryID = strong::type<std::int32_t, struct time_category_id_, strong::equality, strong::incrementable, strong::ordered, strong::partially_ordered>;

template <>
struct std::formatter<TimeCategoryID> : std::formatter<std::int32_t> {
	auto format(TimeCategoryID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};

struct TimeEntry
{
	TimeCategoryID categoryID;
	TimeCodeID codeID;

	constexpr auto operator<=>(const TimeEntry&) const = default;
};

struct TaskTimes
{
	std::chrono::milliseconds start = std::chrono::milliseconds(0);
	std::optional<std::chrono::milliseconds> stop;
	std::vector<TimeEntry> timeEntry;
};

struct TimeCode
{
	TimeCodeID id; // the ID will be continuously incremented, even when deleting time codes that were just created
	std::string name;
	bool inUse = false;
	std::int32_t taskCount;
	bool archived = false;
	
	friend std::ostream& operator<<(std::ostream& out, const TimeCode& code)
	{
		out << "TimeCode { id: " << code.id._val << ", name: " << code.name << ", archived: " << code.archived << " }";

		return out;
	}

	constexpr auto operator<=>(const TimeCode&) const = default;
};

struct TimeCategory
{
	TimeCategoryID id; // the ID will be continuously incremented, even when deleting time categories that were just created
	std::string name;
	std::string label;
	std::vector<TimeCode> codes;
	bool inUse = false;
	std::int32_t taskCount;
	
	bool archived = false;

	friend std::ostream& operator<<(std::ostream& out, const TimeCategory& category)
	{
		out << "TimeCategory { id: " << category.id._val << ", name: " << category.name << ", label: " << category.label << ", inUse: " << category.inUse << ", taskCount: " << category.taskCount << ", archived: " << category.archived << '\n';
		
		for (auto&& code : category.codes)
		{
			out << code << '\n';
		}

		out << " }";

		return out;
	}

	constexpr auto operator<=>(const TimeCategory&) const = default;
};

struct MessageVisitor;

// TODO redo numbers when we're done
enum class PacketType : std::int32_t
{
	VERSION_REQUEST = 1,
	VERSION = 2,

	CREATE_TASK = 3,
	MOVE_TASK = 4,
	START_TASK = 5,
	STOP_TASK = 6,
	FINISH_TASK = 7,
	UPDATE_TASK = 15,
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

	TIME_ENTRY_REQUEST = 26,
	TIME_ENTRY_DATA = 27,
	TIME_ENTRY_MODIFY = 28,

	START_UNKNOWN_TASK = 29,
	ASSIGN_UNKNOWN_TASK = 30,
};

struct Message
{
public:
	Message(PacketType packetType) : m_packetType(packetType) {}

	PacketType packetType() const { return m_packetType; }

	virtual bool operator==(const Message& message) const = 0;

	virtual std::vector<std::byte> pack() const = 0;

	virtual std::ostream& print(std::ostream& out) const = 0;

	friend std::ostream& operator<<(std::ostream& out, const Message& message)
	{
		return message.print(out);
	}

private:
	PacketType m_packetType;
};

inline std::ostream& Message::print(std::ostream& out) const
{
	out << "packetType: " << static_cast<std::int32_t>(m_packetType);
	return out;
}

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

	std::vector<std::byte> pack() const override;
	static std::expected<RequestMessage, UnpackError> unpack(std::span<const std::byte> data);

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
	std::vector<std::string> labels;
	std::vector<TimeEntry> timeEntry;

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
		return parentID == message.parentID && requestID == message.requestID && name == message.name && labels == message.labels && timeEntry == message.timeEntry;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<CreateTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "CreateTaskMessage { ";
		RequestMessage::print(out);
		out << ", parentID: " << parentID._val << ", name: \"" << name << '"';
		out << ", labels { ";
		for (auto&& label : labels)
		{
			out << '"' << label << '"' << ", ";
		}
		out << "}";
		out << ", timeCodes: [ ";
		for (auto time : timeEntry)
		{
			out << std::format("[ {} {} ]", time.categoryID._val, time.codeID._val) << ", ";
		}
		out << "] }";

		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const CreateTaskMessage& message)
	{
		return message.print(out);
	}
};

struct UpdateTaskMessage : RequestMessage
{
	TaskID taskID;
	TaskID parentID;
	bool serverControlled = false;
	bool locked = false;
	std::string name;
	std::vector<std::string> labels;
	std::vector<TimeEntry> timeEntry;

	UpdateTaskMessage(RequestID requestID, TaskID taskID, TaskID parentID, std::string name) : RequestMessage(PacketType::UPDATE_TASK, requestID), taskID(taskID), parentID(parentID), name(std::move(name)) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const UpdateTaskMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const UpdateTaskMessage& message) const
	{
		return requestID == message.requestID && taskID == message.taskID && parentID == message.parentID && name == message.name && labels == message.labels && timeEntry == message.timeEntry;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<UpdateTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "UpdateTaskMessage { ";
		RequestMessage::print(out);
		out << ", taskID: " << taskID._val << ", parentID: " << parentID._val << ", serverControlled: " << serverControlled << ", locked: " << locked << ", name: \"" << name << "\"";
		out << ", labels { ";
		for (auto&& label : labels)
		{
			out << '"' << label << '"' << ", ";
		}
		out << "}";
		out << ", timeCodes: [ ";
		for (auto time : timeEntry)
		{
			out << std::format("[ {} {} ]", time.categoryID, time.codeID) << ", ";
		}
		out << "] }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const UpdateTaskMessage& message)
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

enum class TimeCategoryModType
{
	ADD = 0,
	UPDATE = 1,
	REMOVE_CATEGORY = 2,
	REMOVE_CODE = 3
};

struct TimeEntryDataPacket : Message
{
	std::vector<TimeCategory> timeCategories;

	TimeEntryDataPacket(std::vector<TimeCategory> timeCategories) : Message(PacketType::TIME_ENTRY_DATA), timeCategories(std::move(timeCategories))
	{
	}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const TimeEntryDataPacket*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const TimeEntryDataPacket& message) const
	{
		return packetType() == message.packetType() && timeCategories == message.timeCategories;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TimeEntryDataPacket, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TimeEntryDataPacket { ";
		Message::print(out);

		if (!timeCategories.empty())
		{
			out << ", ";
		}

		for (auto&& category : timeCategories)
		{
			out << category;
		}
		out << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const TimeEntryDataPacket& message)
	{
		message.print(out);
		return out;
	}
};

struct TimeEntryModifyPacket : RequestMessage
{
	TimeCategoryModType type;
	std::vector<TimeCategory> timeCategories;

	TimeEntryModifyPacket(RequestID requestID, TimeCategoryModType type, std::vector<TimeCategory> timeCategories) : RequestMessage(PacketType::TIME_ENTRY_MODIFY, requestID), type(type), timeCategories(std::move(timeCategories))
	{
	}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const TimeEntryModifyPacket*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const TimeEntryModifyPacket& message) const
	{
		return packetType() == message.packetType() && requestID == message.requestID && type == message.type && timeCategories == message.timeCategories;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TimeEntryModifyPacket, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TimeEntryModifyPacket { ";
		RequestMessage::print(out);
		
		out << ", type: " << static_cast<int>(type);

		if (!timeCategories.empty())
		{
			out << ", ";
		}
		
		for (auto&& category : timeCategories)
		{
			out << category;
		}
		out << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const TimeEntryModifyPacket& message)
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

struct TaskInfoMessage : Message
{
	TaskID taskID;
	TaskID parentID;
	TaskState state = TaskState::INACTIVE;
	bool newTask = false;
	bool serverControlled = false;
	bool locked = false;

	std::string name;

	std::chrono::milliseconds createTime = std::chrono::milliseconds(0);
	std::optional<std::chrono::milliseconds> finishTime;
	std::vector<TaskTimes> times;
	
	std::vector<std::string> labels;
	std::vector<TimeEntry> timeEntry;

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

			if (times[i].timeEntry.size() != message.times[i].timeEntry.size())
			{
				return false;
			}

			for (std::size_t j = 0; j < times[i].timeEntry.size(); j++)
			{
				if (times[i].timeEntry[j] != message.times[i].timeEntry[j])
				{
					return false;
				}
			}
		}

		if (labels.size() != message.labels.size())
		{
			return false;
		}

		for (std::size_t i = 0; i < labels.size(); i++)
		{
			if (labels[i] != message.labels[i])
			{
				return false;
			}
		}

		if (timeEntry.size() != message.timeEntry.size())
		{
			return false;
		}

		for (std::size_t i = 0; i < timeEntry.size(); i++)
		{
			if (timeEntry[i].categoryID != message.timeEntry[i].categoryID ||
				timeEntry[i].codeID != message.timeEntry[i].codeID)
			{
				return false;
			}
		}

		return taskID == message.taskID && 
			parentID == message.parentID && 
			state == message.state && 
			newTask == message.newTask && 
			serverControlled == message.serverControlled &&
			locked == message.locked &&
			name == message.name && 
			createTime == message.createTime && 
			finishTime == message.finishTime;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TaskInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TaskInfoMessage { taskID: " << taskID._val << ", parentID: " << parentID._val << ", state: " << static_cast<std::int32_t>(state) << ", newTask: " << newTask << ", serverControlled: " << serverControlled << ", locked: " << locked << ", name: \"" << name << "\", createTime: " << createTime.count() << ", finishTime: " << (finishTime.has_value() ? std::to_string(finishTime.value().count()) : "nullopt") << ", times: [";
		for (auto&& time : times)
		{
			out << "{ start: " << time.start.count() << ", stop: " << (time.stop.has_value() ? std::to_string(time.stop.value().count()) : "nullopt");
			out << ", time codes: [ ";
			for (auto&& code : time.timeEntry)
			{
				out << std::format("[ {} {} ]", code.categoryID._val, code.codeID._val);
				out << ", ";
			}
			out << "]";
			out << " }, ";
		}
		out << "]\n";
		out << "labels: [ ";
		for (auto&& label : labels)
		{
			out << label;
			out << ", ";
		}
		out << "]\n";
		out << "time codes: [ ";
		for (auto&& code : timeEntry)
		{
			out << std::format("[ {} {} ]", code.categoryID, code.codeID);
			out << ", ";
		}
		out << "]";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const TaskInfoMessage& message)
	{
		return message.print(out);
	}
};

struct BugzillaInfoMessage : Message
{
	std::string URL;
	std::string apiKey;
	std::string username;
	TaskID rootTaskID = NO_PARENT;
	std::string groupTasksBy;
	std::map<std::string, std::string> labelToField;

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
		return URL == message.URL && apiKey == message.apiKey && username == message.username && rootTaskID == message.rootTaskID &&
			groupTasksBy == message.groupTasksBy && labelToField == message.labelToField;
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

struct RequestDailyReportMessage : RequestMessage
{
	int month;
	int day;
	int year;

	RequestDailyReportMessage(RequestID requestID, int month, int day, int year) : RequestMessage(PacketType::REQUEST_DAILY_REPORT, requestID), month(month), day(day), year(year)
	{
	}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const RequestDailyReportMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const RequestDailyReportMessage& message) const
	{
		return packetType() == message.packetType() && requestID == message.requestID && month == message.month && day == message.day && year == message.year;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<RequestDailyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "RequestDailyReportMessage { ";
		RequestMessage::print(out);
		out << ", month: " << month << ", day: " << day << ", year: " << year << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const RequestDailyReportMessage& message)
	{
		message.print(out);
		return out;
	}
};

struct RequestWeeklyReportMessage : RequestMessage
{
	int month;
	int day;
	int year;

	RequestWeeklyReportMessage(RequestID requestID, int month, int day, int year) : RequestMessage(PacketType::REQUEST_WEEKLY_REPORT, requestID), month(month), day(day), year(year)
	{
	}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const RequestWeeklyReportMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const RequestWeeklyReportMessage& message) const
	{
		return packetType() == message.packetType() && requestID == message.requestID && month == message.month && day == message.day && year == message.year;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<RequestWeeklyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "RequestWeeklyReportMessage { ";
		RequestMessage::print(out);
		out << ", month: " << month << ", day: " << day << ", year: " << year << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const RequestWeeklyReportMessage& message)
	{
		message.print(out);
		return out;
	}
};

struct DailyReport
{
	bool found = false;

	int month = 0;
	int day = 0;
	int year = 0;

	std::vector<TaskID> tasksCreated;
	std::vector<TaskID> tasksFinished;

	// start time for the day
	std::chrono::milliseconds startTime = std::chrono::milliseconds(0);

	// estimated end time for the day (assuming 8 hours)
	bool estimatedEndTime = false;
	std::chrono::milliseconds endTime = std::chrono::milliseconds(0);

	// list of task ids and the start/stop index
	struct TimePair
	{
		TaskID taskID;
		std::int32_t startStopIndex;

		constexpr auto operator<=>(const TimePair&) const = default;
	};

	std::vector<TimePair> times;

	// total time for the day
	std::chrono::milliseconds totalTime = std::chrono::milliseconds(0);

	// total time per time category for the day
	std::map<TimeEntry, std::chrono::milliseconds> timePerTimeEntry;

	constexpr auto operator<=>(const DailyReport&) const = default;

	friend std::ostream& operator<<(std::ostream& out, const DailyReport& report)
	{
		if (!report.found)
		{
			out << "{ found: " << report.found << ", month: " << report.month << ", day: " << report.day << ", year: " << report.year << " }";
			return out;
		}
		out << "{ found: " << report.found << ", month: " << report.month << ", day: " << report.day << ", year: " << report.year << ", startTime: " << report.startTime << ", endTime: " << report.endTime;
		out << '\n';
		out << "Time Pairs {";
		for (auto&& time : report.times)
		{
			out << "\ntaskID: " << time.taskID._val << ", startStopIndex: " << time.startStopIndex;
		}
		out << "\n}\n";
		out << "Time Per Time Code {";
		for (auto&& [timeEntry, time] : report.timePerTimeEntry)
		{
			out << "\ntimeEntry: " << std::format("[ {} {} ]", timeEntry.categoryID, timeEntry.codeID) << ", time: " << time;
		}
		out << "\n}\n";
		out << "Total Time: " << report.totalTime << '\n';
		out << "}";
		return out;
	}
};

struct DailyReportMessage : Message
{
	RequestID requestID;

	DailyReport report;

	DailyReportMessage(RequestID requestID) : Message(PacketType::DAILY_REPORT), requestID(requestID) {}

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
		return requestID == message.requestID && report == message.report;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<DailyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "DailyReportMessage { ";
		Message::print(out);
		out << ", requestID: " << requestID._val;
		out << ", report: " << report;
		out << "}";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const DailyReportMessage& message)
	{
		message.print(out);
		return out;
	}
};

struct WeeklyReportMessage : Message
{
	RequestID requestID;

	std::array<DailyReport, 7> dailyReports;

	std::chrono::milliseconds totalTime = std::chrono::milliseconds(0);
	std::map<TimeCodeID, std::chrono::milliseconds> timePerTimeCode;

	WeeklyReportMessage(RequestID requestID) : Message(PacketType::WEEKLY_REPORT), requestID(requestID) {}

	bool operator==(const Message& message) const override
	{
		if (const auto* other = dynamic_cast<const WeeklyReportMessage*>(&message))
		{
			return *this == *other;
		}
		return false;
	}

	bool operator==(const WeeklyReportMessage& message) const
	{
		return requestID == message.requestID;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<WeeklyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "WeeklyReportMessage { ";
		Message::print(out);
		out << ", requestID: " << requestID._val << ", totalTime: " << totalTime << ", ";
		for (auto&& dailyReport : dailyReports)
		{
			out << dailyReport;
		}
		out << "Time Per Time Code {";
		for (auto&& [timeCode, time] : timePerTimeCode)
		{
			out << "\ntimeCode: " << timeCode._val << ", time: " << time;
		}
		out << "}";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const WeeklyReportMessage& message)
	{
		message.print(out);
		return out;
	}

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

	void add(bool value)
	{
		add(static_cast<std::int8_t>(value));
	}

	template<typename T>
		requires strong::is_strong_type<T>::value
	void add(T value)
	{
		add(value._val);
	}

	void add(std::chrono::milliseconds value)
	{
		add(static_cast<std::int64_t>(value.count()));
	}

	void add(const std::string& str)
	{
		add(std::string_view(str));
	}

	void add(std::string_view str)
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

	template<typename T>
	auto parse_next_immediate()
	{
		return parse_next<T>().value();
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
		case UPDATE_TASK:
			result.packet = std::make_unique<UpdateTaskMessage>(UpdateTaskMessage::unpack(bytes.subspan(4)).value());
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
		{
			result.packet = std::make_unique<BasicMessage>(BasicMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;

			break;
		}
		case BUGZILLA_REFRESH:
		{
			result.packet = std::make_unique<RequestMessage>(RequestMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		}
		case BUGZILLA_INFO:
		{
			result.packet = std::make_unique<BugzillaInfoMessage>(BugzillaInfoMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;

			break;
		}
		case REQUEST_DAILY_REPORT:
			result.packet = std::make_unique<RequestDailyReportMessage>(RequestDailyReportMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case DAILY_REPORT:
			result.packet = std::make_unique<DailyReportMessage>(DailyReportMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case REQUEST_WEEKLY_REPORT:
			result.packet = std::make_unique<RequestWeeklyReportMessage>(RequestWeeklyReportMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case TIME_ENTRY_DATA:
			result.packet = std::make_unique<TimeEntryDataPacket>(TimeEntryDataPacket::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case TIME_ENTRY_MODIFY:
			result.packet = std::make_unique<TimeEntryModifyPacket>(TimeEntryModifyPacket::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		default:
			break;
		}
	}
	return result;
}

#endif
