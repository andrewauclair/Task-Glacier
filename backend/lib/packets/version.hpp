#pragma once

#include "message.hpp"
#include "unpack_error.hpp"

#include <string>
#include <expected>
#include <cstddef>

struct VersionMessage : Message
{
	std::string version;
	VersionMessage(std::string version) : Message(PacketType::VERSION), version(version) {}

	std::vector<std::byte> pack() const override;
	static std::expected<VersionMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "VersionMessage { " << version << " } ";

		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const VersionMessage& message)
	{
		return message.print(out);
	}
};
