#ifndef MICRO_TASK_PACKETS_HPP
#define MICRO_TASK_PACKETS_HPP

#include "lib.hpp"

#include <vector>
#include <variant>
#include <string>

struct UnpackError {
	enum {
		NOT_ENOUGH_BYTES
	};
};

struct CreateListMessage
{
	GroupID groupID;
	std::string name;

	std::vector<std::byte> pack() const;

	static std::expected<CreateListMessage, UnpackError> unpack(std::span<const std::byte> data);
};

struct CreateGroupMessage
{
	GroupID groupID;
	std::string name;

	std::vector<std::byte> pack() const;
};

using MessageTypes = std::variant<CreateListMessage, CreateGroupMessage>;

class PacketBuilder
{
private:
	std::vector<std::byte> m_bytes;

public:

	std::vector<std::byte> build()
	{
		std::int32_t length = m_bytes.size();
		length += sizeof(length);

		auto* ptr = reinterpret_cast<std::byte*>(&length);

		// inserting in this order will do the byte swapping for us atm
		for (int i = 0; i < sizeof(length); i++, ptr++)
		{
			m_bytes.insert(m_bytes.begin(), *ptr);
		}
		return m_bytes;
	}

	// TODO add specialization for enums
	template<typename T>
	void add_value(T value)
	{
		T swapped = std::byteswap(value);
		auto* f = reinterpret_cast<std::byte*>(&swapped);

		for (int i = 0; i < sizeof(T); i++, f++)
		{
			m_bytes.push_back(*f);
		}
	}

	void add_string(std::string_view str)
	{
		std::int16_t size = str.size();

		add_value(size);

		for (auto ch : str)
		{
			m_bytes.push_back(static_cast<std::byte>(ch));
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

//struct CreateListPacket
//{
//	std::string name;
//	GroupID groupID;
//};
//
//struct CreateGroupPacket
//{
//	std::string name;
//	GroupID groupID;
//};
//
//using PacketTypes = std::variant<CreateListPacket, CreateGroupPacket, std::monostate>;

struct ParseResult
{
	std::optional<MessageTypes> packet;
	std::int32_t bytes_read = 0;
};

inline ParseResult parse_packet(std::span<const std::byte> bytes)
{
	ParseResult result;

	if (bytes.size() > 4)
	{
		std::int32_t raw_length;
		std::memcpy(&raw_length, bytes.data(), sizeof(raw_length));
		raw_length = std::byteswap(raw_length);

		result.bytes_read += sizeof(raw_length);

		// read out the packet type
		std::int32_t raw_type;
		std::memcpy(&raw_type, bytes.data() + result.bytes_read, sizeof(PacketType));
		const PacketType type = static_cast<PacketType>(std::byteswap(raw_type));

		result.bytes_read += sizeof(PacketType);

		switch (type)
		{
			using enum PacketType;

		case CREATE_TASK:
			break;
		case CREATE_LIST:
		{
			result.packet = CreateListMessage::unpack(bytes.subspan(8)).value();
			result.bytes_read = raw_length;

			break;
		}
		case CREATE_GROUP:
		{
			CreateGroupMessage create_group;

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
