#pragma once

#include "task_id.hpp"
#include "request_id.hpp"

#include <map>
#include <vector>
#include <chrono>

struct DailyReport
{
	bool found = false;

	int month = 0;
	int day = 0;
	int year = 0;

	std::vector<TaskID> tasksCreated;
	std::vector<TaskID> tasksFinished;

	// start time for the day
	std::chrono::milliseconds startTime = std::chrono::milliseconds(0);

	// estimated end time for the day (assuming 8 hours)
	bool estimatedEndTime = false;
	std::chrono::milliseconds endTime = std::chrono::milliseconds(0);

	// list of task ids and the start/stop index
	struct TimePair
	{
		TaskID taskID;
		std::int32_t startStopIndex;

		constexpr auto operator<=>(const TimePair&) const = default;
	};

	std::vector<TimePair> times;

	// total time for the day
	std::chrono::milliseconds totalTime = std::chrono::milliseconds(0);

	// total time per time category for the day
	std::map<TimeEntry, std::chrono::milliseconds> timePerTimeEntry;

	constexpr auto operator<=>(const DailyReport&) const = default;

	friend std::ostream& operator<<(std::ostream& out, const DailyReport& report)
	{
		if (!report.found)
		{
			out << "{ found: " << report.found << ", month: " << report.month << ", day: " << report.day << ", year: " << report.year << " }";
			return out;
		}
		out << "{ found: " << report.found << ", month: " << report.month << ", day: " << report.day << ", year: " << report.year << ", startTime: " << report.startTime << ", endTime: " << report.endTime;
		out << '\n';
		out << "Time Pairs {";
		for (auto&& time : report.times)
		{
			out << "\ntaskID: " << time.taskID._val << ", startStopIndex: " << time.startStopIndex;
		}
		out << "\n}\n";
		out << "Time Per Time Code {";
		for (auto&& [timeEntry, time] : report.timePerTimeEntry)
		{
			out << "\ntimeEntry: " << std::format("[ {} ({}) {} ({}) ]", timeEntry.category.name, timeEntry.category.id, timeEntry.code.name, timeEntry.code.id) << ", time: " << time;
		}
		out << "\n}\n";
		out << "Total Time: " << report.totalTime << '\n';
		out << "}";
		return out;
	}
};

struct DailyReportMessage : Message
{
	RequestID requestID;

	std::chrono::milliseconds reportTime;

	DailyReport report;

	DailyReportMessage(RequestID requestID, std::chrono::milliseconds reportTime) : Message(PacketType::DAILY_REPORT), requestID(requestID), reportTime(reportTime) {}

	std::vector<std::byte> pack() const override;
	static std::expected<DailyReportMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "DailyReportMessage { ";
		Message::print(out);
		out << ", requestID: " << requestID._val;
		out << ", reportTime: " << std::format("{:%m/%d/%y %H:%M:%S}", std::chrono::system_clock::time_point{ reportTime });
		out << ", report: " << report;
		out << "}";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const DailyReportMessage& message)
	{
		message.print(out);
		return out;
	}
};
