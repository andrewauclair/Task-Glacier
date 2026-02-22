#pragma once

#include "request.hpp"
#include "task_id.hpp"
#include "task_times.hpp"
#include "time_entry.hpp"
#include "task_state.hpp"

#include <expected>
#include <format>
#include <ostream>
#include <vector>

struct UpdateTaskMessage : RequestMessage
{
	TaskID taskID;
	TaskID parentID;
	TaskState state = TaskState::PENDING;
	std::int32_t indexInParent = 0;
	bool serverControlled = false;
	bool locked = false;
	std::string name;
	std::vector<std::string> labels;
	std::vector<TimeEntry> timeEntry;

	UpdateTaskMessage(RequestID requestID, TaskID taskID, TaskID parentID, std::string name) : RequestMessage(PacketType::UPDATE_TASK, requestID), taskID(taskID), parentID(parentID), name(std::move(name)) {}

	bool operator==(const UpdateTaskMessage& message) const
	{
		return requestID == message.requestID && taskID == message.taskID && parentID == message.parentID && state == message.state && indexInParent == message.indexInParent && name == message.name && labels == message.labels && timeEntry == message.timeEntry;
	}

	std::vector<std::byte> pack() const override;
	static std::expected<UpdateTaskMessage, UnpackError> unpack(std::span<const std::byte> data, const TimeCategories& time_categories);

	std::ostream& print(std::ostream& out) const override
	{
		out << "UpdateTaskMessage { ";
		RequestMessage::print(out);
		out << ", taskID: " << taskID._val << ", parentID: " << parentID._val << ", state: " << static_cast<int>(state) << ", indexInParent: " << indexInParent << ", serverControlled: " << serverControlled << ", locked: " << locked << ", name: \"" << name << "\"";
		out << ", labels { ";
		for (auto&& label : labels)
		{
			out << '"' << label << '"' << ", ";
		}
		out << "}";
		out << ", timeCodes: [ ";
		for (auto&& time : timeEntry)
		{
			out << std::format("[ {} ({}) {} ({}) ]", time.category.name, time.category.id, time.code.name, time.code.id) << ", ";
		}
		out << "] }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const UpdateTaskMessage& message)
	{
		return message.print(out);
	}
};
