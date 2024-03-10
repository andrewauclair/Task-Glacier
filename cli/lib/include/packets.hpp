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

class PacketBuilder
{

};

#endif
