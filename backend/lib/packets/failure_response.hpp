#pragma once

struct FailureResponse : Message
{
	RequestOrigin request;
	std::string message;

	FailureResponse(RequestOrigin request, std::string message) : Message(PacketType::FAILURE_RESPONSE), request(request), message(std::move(message)) {}

	std::vector<std::byte> pack() const override;
	static std::expected<FailureResponse, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "FailureResponse { ";
		Message::print(out);
		out << ", request: " << request << ", message: \"" << message << "\" }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const FailureResponse& message)
	{
		message.print(out);
		return out;
	}
};
