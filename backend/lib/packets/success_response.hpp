#pragma once

struct SuccessResponse : Message
{
	RequestID requestID;

	SuccessResponse(RequestID requestID) : Message(PacketType::SUCCESS_RESPONSE), requestID(requestID) {}

	std::vector<std::byte> pack() const override;
	static std::expected<SuccessResponse, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "SuccessResponse { ";
		Message::print(out);
		out << ", requestID: " << requestID._val << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const SuccessResponse& message)
	{
		message.print(out);
		return out;
	}
};
