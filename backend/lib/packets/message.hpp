#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

#include <magic_enum/magic_enum.hpp>

#include "request_id.hpp"

enum class PacketType : std::int32_t
{
	VERSION_REQUEST = 1,
	VERSION = 2,

	CREATE_TASK = 3,
	MOVE_TASK = 4,
	START_TASK = 5,
	STOP_TASK = 6,
	FINISH_TASK = 7,
	UPDATE_TASK = 8,
	REQUEST_TASK = 9,

	EDIT_TASK_SESSION = 10,
	ADD_TASK_SESSION = 11,
	REMOVE_TASK_SESSION = 12,
	
	SUCCESS_RESPONSE = 13,
	FAILURE_RESPONSE = 14,

	REQUEST_CONFIGURATION = 15,
	REQUEST_CONFIGURATION_COMPLETE = 16,

	TASK_INFO = 17,
	TASK_STATE_CHANGE = 42, // TODO remove. The UI is using it for the unspecified task?

	// configure and refresh bugzilla (refresh is manual by the user on the UI)
	// the server will create and update tasks based on bugzilla changes and send TASK_INFO messages
	// these messages have responses of SUCCESS_RESPONSE or FAILURE_RESPONSE
	BUGZILLA_INFO = 18,
	BUGZILLA_REFRESH = 19,
	BUGZILLA_REFRESH_COMPLETE = 43,
	BUGZILLA_INFO_MODIFY = 44,
	BUGZILLA_INFO_REMOVE = 45,

	// report for the day with various information, displayed on a dialog on the UI
	DAILY_REPORT = 20,
	REQUEST_DAILY_REPORT = 21,

	// report for the time tracking for the week, displayed on a dialog on the UI
	WEEKLY_REPORT = 22,
	REQUEST_WEEKLY_REPORT = 23,

	// configure the backup, including the path of the backup folder, how often to perform a backup, and how many backups to keep
	BACKUP_CONFIGURATION = 26,
	// backup has been successfully performed
	BACKUP_PERFORMED = 27,
	// backup has failed. error message and last successful backup time are provided
	BACKUP_FAILED = 28,

	REQUEST_TIME_ENTRY = 29,
	TIME_ENTRY_DATA = 30,
	TIME_ENTRY_MODIFY = 31,

	START_UNSPECIFIED_TASK = 32,
	STOP_UNSPECIFIED_TASK = 33,
	UNSPECIFIED_TASK_ACTIVE = 34,

	BULK_TASK_UPDATE_START = 35,
	BULK_TASK_UPDATE_FINISH = 36,

	BULK_TASK_INFO_START = 37,
	BULK_TASK_INFO_FINISH = 38,

	BULK_TASK_ADD_START = 39,
	BULK_TASK_ADD_FINISH = 40,

	ERROR_MESSAGE = 41,
};

struct RequestOrigin
{
	PacketType packetType;
	RequestID id;

	constexpr bool operator==(const RequestOrigin& other) const
	{
		return id == other.id;
	}

	constexpr bool operator!=(const RequestOrigin& other) const
	{
		return id != other.id;
	}

	constexpr bool operator<(const RequestOrigin& other) const
	{
		return id < other.id;
	}

	friend std::ostream& operator<<(std::ostream& out, RequestOrigin request)
	{
		out << request.id._val << " (" << magic_enum::enum_name(request.packetType) << ")";
		return out;
	}
};

struct Message
{
public:
	Message(PacketType packetType) : m_packetType(packetType) {}

	PacketType packetType() const { return m_packetType; }

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
	out << "packetType: " << magic_enum::enum_name(m_packetType) << " (" << static_cast<std::int32_t>(m_packetType) << ")";
	return out;
}
