#pragma once

#include "request.hpp"

#include "request_id.hpp"
#include "task_id.hpp"
#include "time_entry.hpp"

#include <string>
#include <expected>

struct CreateTaskMessage : RequestMessage
{
	TaskID parentID;
	std::string name;
	std::vector<std::string> labels;
	std::vector<TimeEntry> timeEntry;

	CreateTaskMessage(TaskID parentID, RequestID requestID, std::string name) : RequestMessage(PacketType::CREATE_TASK, requestID), parentID(parentID), name(std::move(name)) {}

	std::vector<std::byte> pack() const override;
	static std::expected<CreateTaskMessage, UnpackError> unpack(std::span<const std::byte> data, const TimeCategories& time_categories);

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
			out << std::format("[ {} ({}) {} ({}) ]", time.category.name, time.category.id._val, time.code.name, time.code.id._val) << ", ";
		}
		out << "] }";

		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const CreateTaskMessage& message)
	{
		return message.print(out);
	}
};