#pragma once

#include "message.hpp"
#include "task_id.hpp"
#include "task_state.hpp"
#include "task_times.hpp"
#include "unpack_error.hpp"

#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include <expected>
#include <span>

struct TaskInfoMessage : Message
{
	TaskID taskID;
	TaskID parentID;
	TaskState state = TaskState::PENDING;
	bool newTask = false;
	std::int32_t indexInParent = 0;
	bool serverControlled = false;
	bool locked = false;

	std::string name;

	std::chrono::milliseconds createTime = std::chrono::milliseconds(0);
	std::optional<std::chrono::milliseconds> finishTime;
	std::vector<TaskTimes> times;

	std::vector<std::string> labels;
	std::vector<TimeEntry> timeEntry;

	TaskInfoMessage(TaskID taskID, TaskID parentID, std::string name, std::chrono::milliseconds createTime = std::chrono::milliseconds(0)) : Message(PacketType::TASK_INFO), taskID(taskID), parentID(parentID), name(std::move(name)), createTime(createTime) {}

	std::vector<std::byte> pack() const override;
	static std::expected<TaskInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TaskInfoMessage { taskID: " << taskID._val << ", parentID: " << parentID._val << ", indexInParent: " << indexInParent << ", state: " << static_cast<std::int32_t>(state) << ", newTask : " << newTask << ", serverControlled : " << serverControlled << ", locked : " << locked << ", name : \"" << name << "\", createTime: " << createTime.count() << ", finishTime: " << (finishTime.has_value() ? std::to_string(finishTime.value().count()) : "nullopt") << ", times: [";
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
