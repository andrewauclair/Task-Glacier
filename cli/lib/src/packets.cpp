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
	int bytes_read = 0;

	std::int32_t raw_request_id;
	std::memcpy(&raw_request_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	const auto requestID = std::byteswap(raw_request_id);

	std::int32_t raw_list_id;
	std::memcpy(&raw_list_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	const auto listID = std::byteswap(raw_list_id);

	std::int16_t raw_name_length;
	std::memcpy(&raw_name_length, data.data() + bytes_read, sizeof(std::int16_t));

	bytes_read += sizeof(std::int16_t);

	auto length = std::byteswap(raw_name_length);
	std::string name;
	name.resize(length);
	std::memcpy(name.data(), data.data() + bytes_read, length);

	return CreateTaskMessage(ListID(listID), RequestID(requestID), name);
}

void CreateListMessage::visit(MessageVisitor& visitor) const { visitor.visit(*this); }

std::vector<std::byte> CreateListMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::CREATE_LIST));
	builder.add(requestID);
	builder.add(groupID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateListMessage, UnpackError> CreateListMessage::unpack(std::span<const std::byte> data)
{
	int bytes_read = 0;

	std::int32_t raw_request_id;
	std::memcpy(&raw_request_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	const auto requestID = std::byteswap(raw_request_id);

	std::int32_t raw_group_id;
	std::memcpy(&raw_group_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	const auto groupID = std::byteswap(raw_group_id);

	std::int16_t raw_name_length;
	std::memcpy(&raw_name_length, data.data() + bytes_read, sizeof(std::int16_t));

	bytes_read += sizeof(std::int16_t);

	auto length = std::byteswap(raw_name_length);
	std::string name;
	name.resize(length);
	std::memcpy(name.data(), data.data() + bytes_read, length);

	return CreateListMessage(GroupID(groupID), RequestID(requestID), name);
}

void CreateGroupMessage::visit(MessageVisitor& visitor) const { visitor.visit(*this); };

std::vector<std::byte> CreateGroupMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::CREATE_GROUP));
	builder.add(requestID);
	builder.add(groupID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateGroupMessage, UnpackError> CreateGroupMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);
	parser.parse_value<std::int16_t>();

	int bytes_read = 0;

	std::int32_t raw_request_id;
	std::memcpy(&raw_request_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	RequestID requestID = RequestID(std::byteswap(raw_request_id));

	std::int32_t raw_group_id;
	std::memcpy(&raw_group_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	GroupID groupID = GroupID(std::byteswap(raw_group_id));

	std::int16_t raw_name_length;
	std::memcpy(&raw_name_length, data.data() + bytes_read, sizeof(std::int16_t));

	bytes_read += sizeof(std::int16_t);

	auto length = std::byteswap(raw_name_length);
	std::string name;
	name.resize(length);
	std::memcpy(name.data(), data.data() + bytes_read, length);

	return CreateGroupMessage(groupID, requestID, name);
}

std::vector<std::byte> SuccessResponse::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::SUCCESS_RESPONSE));
	builder.add(requestID);

	return builder.build();
}

std::vector<std::byte> FailureResponse::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::FAILURE_RESPONSE));
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

	builder.add(static_cast<std::int32_t>(packetType));

	return builder.build();
}

std::expected<EmptyMessage, UnpackError> EmptyMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);
	parser.parse_value<std::int16_t>();

	int bytes_read = 0;

	std::int32_t raw_packet_id;
	std::memcpy(&raw_packet_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	PacketType packetType = PacketType(std::byteswap(raw_packet_id));

	return EmptyMessage(packetType);
}