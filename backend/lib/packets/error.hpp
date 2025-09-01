#pragma once

struct ErrorMessage : Message
{
	std::string message;

	ErrorMessage(std::string message) : Message(PacketType::ERROR_MESSAGE), message(std::move(message)) {}

	std::vector<std::byte> pack() const override;
	static std::expected<ErrorMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "ErrorMessage { ";
		Message::print(out);
		out << ", message: \"" << message << "\" }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const ErrorMessage& message)
	{
		message.print(out);
		return out;
	}
};
