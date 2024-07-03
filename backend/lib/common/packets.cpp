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
	builder.add(parentID);
	builder.add_string(name);

	return builder.build();
}

std::expected<TaskInfoMessage, UnpackError> TaskInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto taskID = parser.parse_next<TaskID>();
	const auto parentID = parser.parse_next<TaskID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return TaskInfoMessage(taskID.value(), parentID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}
