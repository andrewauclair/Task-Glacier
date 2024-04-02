#include "packets.hpp"
#include "lib.hpp"

void CreateTaskMessage::visit(MessageVisitor& visitor) const { visitor.visit(*this); }

std::vector<std::byte> CreateTaskMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::CREATE_TASK));
	builder.add(requestID);
	builder.add(listID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateTaskMessage, UnpackError> CreateTaskMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);
	
	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto listID = parser.parse_next<ListID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return CreateTaskMessage(listID.value(), requestID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

void CreateListMessage::visit(MessageVisitor& visitor) const { visitor.visit(*this); }

std::vector<std::byte> CreateListMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::CREATE_LIST);
	builder.add(requestID);
	builder.add(groupID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateListMessage, UnpackError> CreateListMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto groupID = parser.parse_next<GroupID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return CreateListMessage(groupID.value(), requestID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

void CreateGroupMessage::visit(MessageVisitor& visitor) const { visitor.visit(*this); };

std::vector<std::byte> CreateGroupMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::CREATE_GROUP);
	builder.add(requestID);
	builder.add(groupID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateGroupMessage, UnpackError> CreateGroupMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto groupID = parser.parse_next<GroupID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return CreateGroupMessage(groupID.value(), requestID.value(), name.value());
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

void EmptyMessage::visit(MessageVisitor& visitor) const
{
	visitor.visit(*this);
}

std::vector<std::byte> EmptyMessage::pack() const
{
	PacketBuilder builder;

	builder.add(packetType);

	return builder.build();
}

std::expected<EmptyMessage, UnpackError> EmptyMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();

	if (packetType)
	{
		return EmptyMessage(packetType.value());
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
	builder.add(listID);
	builder.add_string(name);

	return builder.build();
}

std::expected<TaskInfoMessage, UnpackError> TaskInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto taskID = parser.parse_next<TaskID>();
	const auto listID = parser.parse_next<ListID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return TaskInfoMessage(taskID.value(), listID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

void ListInfoMessage::visit(MessageVisitor& visitor) const
{
	visitor.visit(*this);
}

std::vector<std::byte> ListInfoMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::LIST_INFO));
	builder.add(groupID);
	builder.add_string(name);

	return builder.build();
}

std::expected<ListInfoMessage, UnpackError> ListInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto groupID = parser.parse_next<GroupID>();
	const auto listID = parser.parse_next<ListID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return ListInfoMessage(groupID.value(), listID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

void GroupInfoMessage::visit(MessageVisitor& visitor) const
{
	visitor.visit(*this);
}

std::vector<std::byte> GroupInfoMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::CREATE_GROUP);
	builder.add(groupID);
	builder.add_string(name);

	return builder.build();
}

std::expected<GroupInfoMessage, UnpackError> GroupInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto groupID = parser.parse_next<GroupID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return GroupInfoMessage(groupID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}