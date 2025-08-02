#pragma once

#include "message.hpp"
#include "unpack_error.hpp"

struct BasicMessage : Message
{
	BasicMessage(PacketType type) : Message(type) {}

	std::vector<std::byte> pack() const override;
	static std::expected<BasicMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "BasicMessage { ";
		Message::print(out);
		out << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const BasicMessage& message)
	{
		message.print(out);
		return out;
	}
};
