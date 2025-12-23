#pragma once

#include "request.hpp"
#include "task_id.hpp"

#include <cassert>

struct TaskMessage : RequestMessage
{
	TaskID taskID;

	TaskMessage(PacketType type, RequestID requestID, TaskID taskID) : RequestMessage(type, requestID), taskID(taskID)
	{
		assert(type == PacketType::START_TASK || 
			   type == PacketType::STOP_TASK || 
			   type == PacketType::FINISH_TASK || 
			   type == PacketType::REQUEST_TASK ||
			   type == PacketType::START_UNSPECIFIED_TASK ||
			   type == PacketType::STOP_UNSPECIFIED_TASK);
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
