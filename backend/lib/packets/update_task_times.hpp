#pragma once

#include "request.hpp"
#include "task_id.hpp"
#include "task_times.hpp"

#include <cassert>

using namespace std::chrono_literals;

struct UpdateTaskTimesMessage : RequestMessage
{
	TaskID taskID;
	std::int32_t sessionIndex = 0; // used for edit/remove
	std::chrono::milliseconds start = std::chrono::milliseconds(0);
	std::optional<std::chrono::milliseconds> stop;

	bool checkForOverlaps = false; // tells the server to check for overlaps and not apply the change

	UpdateTaskTimesMessage(PacketType type, RequestID requestID, TaskID taskID, std::chrono::milliseconds start, std::optional<std::chrono::milliseconds> stop) : RequestMessage(type, requestID), taskID(taskID), start(start), stop(stop)
	{
		assert(type == PacketType::ADD_TASK_SESSION || type == PacketType::EDIT_TASK_SESSION || type == PacketType::REMOVE_TASK_SESSION);
	}

	std::vector<std::byte> pack() const;
	static std::expected<UpdateTaskTimesMessage, UnpackError> unpack(std::span<const std::byte> data);
};
