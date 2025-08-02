#pragma once

#include "message.hpp"

#include "request_id.hpp"
#include "unpack_error.hpp"

#include <cstddef>
#include <expected>
#include <ostream>
#include <span>
#include <vector>

struct RequestMessage : Message
{
	RequestID requestID;

	RequestMessage(PacketType packetType, RequestID requestID) : Message(packetType), requestID(requestID) {}

	std::vector<std::byte> pack() const override;
	static std::expected<RequestMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		Message::print(out);
		out << ", requestID: " << requestID._val;
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const RequestMessage& message)
	{
		return message.print(out);
	}
};
