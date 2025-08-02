#pragma once

struct FailureResponse : Message
{
	RequestID requestID;
	std::string message;

	FailureResponse(RequestID requestID, std::string message) : Message(PacketType::FAILURE_RESPONSE), requestID(requestID), message(std::move(message)) {}

	std::vector<std::byte> pack() const override;
	static std::expected<FailureResponse, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "FailureResponse { ";
		Message::print(out);
		out << ", requestID: " << requestID._val << ", message: \"" << message << "\" }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const FailureResponse& message)
	{
		message.print(out);
		return out;
	}
};
