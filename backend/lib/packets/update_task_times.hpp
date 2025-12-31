#pragma once

#include "request.hpp"
#include "task_id.hpp"
#include "task_times.hpp"

#include <cassert>

struct UpdateTaskTimesMessage : RequestMessage
{
	TaskID taskID;
	std::int32_t sessionIndex = 0; // used for edit/remove
	TaskTimes times;

	UpdateTaskTimesMessage(PacketType type, RequestID requestID, TaskID taskID, TaskTimes times) : RequestMessage(type, requestID), taskID(taskID), times(times)
	{
		assert(type == PacketType::ADD_TASK_SESSION || type == PacketType::EDIT_TASK_SESSION || type == PacketType::REMOVE_TASK_SESSION);
	}
};
