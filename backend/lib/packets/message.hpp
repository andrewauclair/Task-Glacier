#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

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

	START_UNSPECIFIED_TASK = 29,
	STOP_UNSPECIFIED_TASK = 30,
	UNSPECIFIED_TASK_ACTIVE = 38,

	BULK_TASK_UPDATE_START = 31,
	BULK_TASK_UPDATE_FINISH = 32,

	BULK_TASK_INFO_START = 33,
	BULK_TASK_INFO_FINISH = 34,

	BULK_TASK_ADD_START = 35,
	BULK_TASK_ADD_FINISH = 36,

	ERROR_MESSAGE = 37,
};

struct Message
{
public:
	Message(PacketType packetType) : m_packetType(packetType) {}

	PacketType packetType() const { return m_packetType; }

	virtual bool operator==(const Message& message) const = delete;// = 0;

	virtual std::vector<std::byte> pack() const = 0;

	virtual std::ostream& print(std::ostream& out) const = 0;

	friend std::ostream& operator<<(std::ostream& out, const Message& message)
	{
		return message.print(out);
	}

private:
	PacketType m_packetType;
};

//inline bool Message::operator==(const Message& message) const
//{
//	return m_packetType == message.m_packetType;
//}

inline std::ostream& Message::print(std::ostream& out) const
{
	out << "packetType: " << static_cast<std::int32_t>(m_packetType);
	return out;
}
