#ifndef MICRO_TASK_PACKETS_HPP
#define MICRO_TASK_PACKETS_HPP

#include "lib.hpp"

#include <vector>
#include <variant>
#include <string>

struct CreateListMessage
{
	GroupID groupID;
	std::string name;

	std::vector<std::byte> pack() const;
};

struct CreateGroupMessage
{
	GroupID groupID;
	std::string name;

	std::vector<std::byte> pack() const;
};

using MessageTypes = std::variant<CreateListMessage, CreateGroupMessage>;

struct PacketBuilder
{
	std::vector<std::byte> bytes;

//public:
	//std::span<const std::byte> bytes() const { return m_bytes; }

	// TODO add specialization for enums
	template<typename T>
	void add_value(T value)
	{
		T swapped = std::byteswap(value);
		auto* f = reinterpret_cast<std::byte*>(&swapped);

		for (int i = 0; i < sizeof(T); i++, f++)
		{
			bytes.push_back(*f);
		}
	}

	void add_string(std::string_view str)
	{
		std::int16_t size = str.size();

		add_value(size);

		for (auto ch : str)
		{
			bytes.push_back(static_cast<std::byte>(ch));
		}
	}
};

enum class PacketType : std::int32_t
{
	CREATE_TASK = 1,
	CREATE_LIST,
	CREATE_GROUP,

	MOVE_TASK,
	MOVE_LIST,
	MOVE_GROUP,

	START_TASK,
	STOP_TASK,
	FINISH_TASK,
};

struct CreateListPacket
{
	std::string name;
	GroupID groupID;
};

struct CreateGroupPacket
{
	std::string name;
	GroupID groupID;
};

using PacketTypes = std::variant<CreateListPacket, CreateGroupPacket, std::monostate>;

struct ParseResult
{
	std::optional<PacketTypes> packet;
	std::int32_t bytes_read = 0;
};

inline ParseResult parse_packet(std::span<const std::byte> bytes)
{
	ParseResult result;

	if (bytes.size() > 4)
	{
		// read out the packet type
		std::int32_t raw_type;
		std::memcpy(&raw_type, bytes.data(), sizeof(PacketType));
		const PacketType type = static_cast<PacketType>(std::byteswap(raw_type));

		result.bytes_read += sizeof(PacketType);

		switch (type)
		{
			using enum PacketType;

		case CREATE_TASK:
			break;
		case CREATE_LIST:
		{
			CreateListPacket create_list;

			std::int32_t raw_group_id;
			std::memcpy(&raw_group_id, bytes.data() + result.bytes_read, sizeof(std::int32_t));

			result.bytes_read += sizeof(std::int32_t);

			create_list.groupID = std::byteswap(raw_group_id);

			std::int16_t raw_name_length;
			std::memcpy(&raw_name_length, bytes.data() + result.bytes_read, sizeof(std::int16_t));

			result.bytes_read += sizeof(std::int16_t);

			auto length = std::byteswap(raw_name_length);
			create_list.name.resize(length);
			std::memcpy(create_list.name.data(), bytes.data() + result.bytes_read, length);

			result.bytes_read += length;

			result.packet = create_list;

			break;
		}
		case CREATE_GROUP:
		{
			CreateGroupPacket create_group;

			std::int32_t raw_group_id;
			std::memcpy(&raw_group_id, bytes.data() + result.bytes_read, sizeof(std::int32_t));

			result.bytes_read += sizeof(std::int32_t);

			create_group.groupID = std::byteswap(raw_group_id);

			std::int16_t raw_name_length;
			std::memcpy(&raw_name_length, bytes.data() + result.bytes_read, sizeof(std::int16_t));

			result.bytes_read += sizeof(std::int16_t);

			auto length = std::byteswap(raw_name_length);
			create_group.name.resize(length);
			std::memcpy(create_group.name.data(), bytes.data() + result.bytes_read, length);

			result.bytes_read += length;

			result.packet = create_group;

			break;
		}
		default:
			break;
		}
	}
	return result;
}

#endif
