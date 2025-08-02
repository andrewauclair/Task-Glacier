#pragma once

struct RequestWeeklyReportMessage : RequestMessage
{
	int month;
	int day;
	int year;

	RequestWeeklyReportMessage(RequestID requestID, int month, int day, int year) : RequestMessage(PacketType::REQUEST_WEEKLY_REPORT, requestID), month(month), day(day), year(year)
	{
	}

	std::vector<std::byte> pack() const override;
	static std::expected<RequestWeeklyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "RequestWeeklyReportMessage { ";
		RequestMessage::print(out);
		out << ", month: " << month << ", day: " << day << ", year: " << year << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const RequestWeeklyReportMessage& message)
	{
		message.print(out);
		return out;
	}
};
