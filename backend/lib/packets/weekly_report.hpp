#pragma once

struct WeeklyReportMessage : Message
{
	RequestID requestID;

	std::chrono::milliseconds reportTime;

	std::array<DailyReport, 7> dailyReports;

	std::chrono::milliseconds totalTime = std::chrono::milliseconds(0);
	std::map<TimeCodeID, std::chrono::milliseconds> timePerTimeCode;

	WeeklyReportMessage(RequestID requestID, std::chrono::milliseconds reportTime) : Message(PacketType::WEEKLY_REPORT), requestID(requestID), reportTime(reportTime) {}

	std::vector<std::byte> pack() const override;
	static std::expected<WeeklyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "WeeklyReportMessage { ";
		Message::print(out);
		out << ", requestID: " << requestID._val << ", totalTime: " << totalTime << ", ";
		for (auto&& dailyReport : dailyReports)
		{
			out << dailyReport;
		}
		out << "Time Per Time Code {";
		for (auto&& [timeCode, time] : timePerTimeCode)
		{
			out << "\ntimeCode: " << timeCode._val << ", time: " << time;
		}
		out << "}";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const WeeklyReportMessage& message)
	{
		message.print(out);
		return out;
	}

	// su, mo, tu, we, th, fr, sa
	// time spent on each time category per day
	// list of task ids and the start/stop index
	// total time per day
	// total time for week
	// total time per time category per day
	// total time per time category for week
};
