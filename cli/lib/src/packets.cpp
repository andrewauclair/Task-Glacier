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

std::expected<CreateListMessage, UnpackError> CreateListMessage::unpack()
{
	CreateListMessage message;

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