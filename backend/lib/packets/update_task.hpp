#pragma once

#include "request.hpp"
#include "task_id.hpp"
#include "task_times.hpp"
#include "time_entry.hpp"

#include <expected>
#include <format>
#include <ostream>
#include <vector>

struct UpdateTaskMessage : RequestMessage
{
	TaskID taskID;
	TaskID parentID;
	std::int32_t indexInParent = 0;
	bool serverControlled = false;
	bool locked = false;
	std::string name;
	std::vector<TaskTimes> times;
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

		return requestID == message.requestID && taskID == message.taskID && parentID == message.parentID && indexInParent == message.indexInParent && name == message.name && labels == message.labels && timeEntry == message.timeEntry;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<UpdateTaskMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "UpdateTaskMessage { ";
		RequestMessage::print(out);
		out << ", taskID: " << taskID._val << ", parentID: " << parentID._val << ", indexInParent: " << indexInParent << ", serverControlled: " << serverControlled << ", locked: " << locked << ", name: \"" << name << "\"";
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
