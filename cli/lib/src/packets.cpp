#include "packets.hpp"
#include "lib.hpp"

std::vector<std::byte> CreateListMessage::pack() const
{
	PacketBuilder builder;

	builder.add_value(static_cast<std::int32_t>(PacketType::CREATE_LIST));
	builder.add_value(groupID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateListMessage, UnpackError> CreateListMessage::unpack(std::span<const std::byte> data)
{
	CreateListMessage message;

	int bytes_read = 0;

	std::int32_t raw_group_id;
	std::memcpy(&raw_group_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	message.groupID = std::byteswap(raw_group_id);

	std::int16_t raw_name_length;
	std::memcpy(&raw_name_length, data.data() + bytes_read, sizeof(std::int16_t));

	bytes_read += sizeof(std::int16_t);

	auto length = std::byteswap(raw_name_length);
	message.name.resize(length);
	std::memcpy(message.name.data(), data.data() + bytes_read, length);

	return message;
}

std::vector<std::byte> CreateGroupMessage::pack() const
{
	PacketBuilder builder;

	builder.add_value(static_cast<std::int32_t>(PacketType::CREATE_GROUP));
	builder.add_value(groupID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateGroupMessage, UnpackError> CreateGroupMessage::unpack(std::span<const std::byte> data)
{
	CreateGroupMessage message;

	int bytes_read = 0;

	std::int32_t raw_group_id;
	std::memcpy(&raw_group_id, data.data() + bytes_read, sizeof(std::int32_t));

	bytes_read += sizeof(std::int32_t);

	message.groupID = std::byteswap(raw_group_id);

	std::int16_t raw_name_length;
	std::memcpy(&raw_name_length, data.data() + bytes_read, sizeof(std::int16_t));

	bytes_read += sizeof(std::int16_t);

	auto length = std::byteswap(raw_name_length);
	message.name.resize(length);
	std::memcpy(message.name.data(), data.data() + bytes_read, length);

	return message;
}