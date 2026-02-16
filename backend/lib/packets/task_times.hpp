#pragma once

#include "time_entry.hpp"

#include <chrono>
#include <optional>
#include <ostream>
#include <vector>

struct TaskTimes
{
	std::chrono::milliseconds start = std::chrono::milliseconds(0);
	std::optional<std::chrono::milliseconds> stop;
	std::vector<TimeEntry> timeEntry;

	constexpr auto operator<=>(const TaskTimes&) const = default;

	friend std::ostream& operator<<(std::ostream& out, const TaskTimes& session)
	{
		out << "Session { start: " << session.start;
		out << ", stop: ";
		if (session.stop)
		{
			out << session.stop.value();
		}
		else
		{
			out << "empty";
		}
		out << ", ";

		for (auto&& time : session.timeEntry)
		{
			out << time;
		}
		out << " }";

		return out;
	}
};

inline TaskTimes create_times_with_unknown_time_entry(std::chrono::milliseconds start, std::optional<std::chrono::milliseconds> stop)
{
	TimeCategory unknownCategory{ TimeCategoryID(0), "Unknown" };
	TimeCode unknownCode{ TimeCodeID(0), "Unknown" };

	return TaskTimes(start, stop, std::vector{ TimeEntry{ unknownCategory, unknownCode } });
}
