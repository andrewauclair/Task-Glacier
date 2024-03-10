#include "packets.hpp"
#include "lib.hpp"

std::vector<std::byte> CreateListMessage::pack() const
{
	std::vector<std::byte> bytes;
	bytes.resize(17);

	std::int32_t type = static_cast<std::int32_t>(PacketType::CREATE_LIST);
	type = std::byteswap(type);

	std::memcpy(bytes.data(), &type, sizeof(std::int32_t));
	
	auto id = std::byteswap(groupID);
	std::memcpy(bytes.data() + 4, &id, sizeof(GroupID));

	std::int16_t length = name.size();
	length = std::byteswap(length);
	std::memcpy(bytes.data() + 8, &length, sizeof(std::int16_t));

	std::memcpy(bytes.data() + 10, name.data(), name.size());
	return bytes;
}

std::vector<std::byte> CreateGroupMessage::pack() const
{
	std::vector<std::byte> bytes;
	bytes.resize(17);
	return bytes;
}