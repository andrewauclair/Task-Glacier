#include "packets.hpp"

void CreateTaskMessage::visit(MessageVisitor& visitor) const { visitor.visit(*this); }

std::vector<std::byte> CreateTaskMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::CREATE_TASK));
	builder.add(requestID);
	builder.add(parentID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateTaskMessage, UnpackError> CreateTaskMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);
	
	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto parentID = parser.parse_next<TaskID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return CreateTaskMessage(parentID.value(), requestID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

void TaskMessage::visit(MessageVisitor& visitor) const { visitor.visit(*this); }

std::vector<std::byte> TaskMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(packetType()));
	builder.add(requestID);
	builder.add(taskID);

	return builder.build();
}

std::expected<TaskMessage, UnpackError> TaskMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto taskID = parser.parse_next<TaskID>();

	try
	{
		return TaskMessage(packetType.value(), requestID.value(), taskID.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> SuccessResponse::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::SUCCESS_RESPONSE);
	builder.add(requestID);

	return builder.build();
}

std::vector<std::byte> FailureResponse::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::FAILURE_RESPONSE);
	builder.add(requestID);
	builder.add_string(message);

	return builder.build();
}

void BasicMessage::visit(MessageVisitor& visitor) const
{
	visitor.visit(*this);
}

std::vector<std::byte> BasicMessage::pack() const
{
	PacketBuilder builder;

	builder.add(packetType);

	return builder.build();
}

std::expected<BasicMessage, UnpackError> BasicMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();

	if (packetType)
	{
		return BasicMessage(packetType.value());
	}
	return std::unexpected(packetType.error());
}

void TaskInfoMessage::visit(MessageVisitor& visitor) const
{
	visitor.visit(*this);
}

std::vector<std::byte> TaskInfoMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::TASK_INFO);
	builder.add(taskID);
	builder.add(parentID);
	builder.add(state);
	builder.add(newTask);
	builder.add_string(name);
	builder.add(createTime);

	builder.add(static_cast<std::int32_t>(times.size()));

	for (auto&& time : times)
	{
		builder.add(time.start);
		builder.add(time.stop.has_value());
		builder.add(time.stop.value_or(std::chrono::milliseconds(0)));
	}

	builder.add(finishTime.has_value());
	builder.add(finishTime.value_or(std::chrono::milliseconds(0)));

	return builder.build();
}

std::expected<TaskInfoMessage, UnpackError> TaskInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto taskID = parser.parse_next<TaskID>();
	const auto parentID = parser.parse_next<TaskID>();
	const auto state = parser.parse_next<TaskState>();
	const auto newTask = parser.parse_next<bool>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		auto info = TaskInfoMessage(taskID.value(), parentID.value(), name.value());
		info.state = state.value();
		info.newTask = newTask.value();

		info.createTime = parser.parse_next<std::chrono::milliseconds>().value();

		const auto startStopCount = parser.parse_next<std::int32_t>().value();

		for (std::int32_t i = 0; i < startStopCount; i++)
		{
			TaskTimes times;

			times.start = parser.parse_next<std::chrono::milliseconds>().value();

			const bool stopPresent = parser.parse_next<bool>().value();

			if (stopPresent)
			{
				times.stop = parser.parse_next<std::chrono::milliseconds>().value();
			}

			info.times.push_back(times);
		}

		const bool finishTimePresent = parser.parse_next<bool>().value();
		
		if (finishTimePresent)
		{
			info.finishTime = parser.parse_next<std::chrono::milliseconds>().value();
		}

		return info;
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

void BugzillaInfoMessage::visit(MessageVisitor& visitor) const
{
	visitor.visit(*this);
}

std::vector<std::byte> BugzillaInfoMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::BUGZILLA_INFO);
	builder.add_string(URL);
	builder.add_string(apiKey);

	return builder.build();
}

std::expected<BugzillaInfoMessage, UnpackError> BugzillaInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto URL = parser.parse_next<std::string>();
	const auto apiKey = parser.parse_next<std::string>();

	try
	{
		return BugzillaInfoMessage(URL.value(), apiKey.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}