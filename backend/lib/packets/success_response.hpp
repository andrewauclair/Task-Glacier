#pragma once

struct SuccessResponse : Message
{
	RequestOrigin request;

	SuccessResponse(RequestOrigin request) : Message(PacketType::SUCCESS_RESPONSE), request(request) {}

	std::vector<std::byte> pack() const override;
	static std::expected<SuccessResponse, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "SuccessResponse { ";
		Message::print(out);
		out << ", request: " << request << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const SuccessResponse& message)
	{
		message.print(out);
		return out;
	}
};
