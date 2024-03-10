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

#endif
