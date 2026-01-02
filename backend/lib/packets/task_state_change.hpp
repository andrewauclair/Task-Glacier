#pragma once

#include "request.hpp"
#include "task_id.hpp"
#include "task_state.hpp"

struct TaskStateChange : RequestMessage
{
	TaskID taskID;
	TaskState state;

	TaskStateChange(RequestID requestID, TaskID taskID, TaskState state) : RequestMessage(PacketType::TASK_STATE_CHANGE, requestID), taskID(taskID), state(state)
	{
	}

	std::vector<std::byte> pack() const;
	static std::expected<TaskStateChange, UnpackError> unpack(std::span<const std::byte> data);
};
