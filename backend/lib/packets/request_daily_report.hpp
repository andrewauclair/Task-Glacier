#pragma once

struct RequestDailyReportMessage : RequestMessage
{
	int month;
	int day;
	int year;

	RequestDailyReportMessage(RequestID requestID, int month, int day, int year) : RequestMessage(PacketType::REQUEST_DAILY_REPORT, requestID), month(month), day(day), year(year)
	{
	}

	std::vector<std::byte> pack() const override;
	static std::expected<RequestDailyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "RequestDailyReportMessage { ";
		RequestMessage::print(out);
		out << ", month: " << month << ", day: " << day << ", year: " << year << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const RequestDailyReportMessage& message)
	{
		message.print(out);
		return out;
	}
};
