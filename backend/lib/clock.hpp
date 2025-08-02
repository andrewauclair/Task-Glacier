#pragma once

#include <chrono>

struct DateTimeRange
{
	std::chrono::milliseconds start;
	std::chrono::milliseconds end;
};

inline std::chrono::milliseconds date_to_ms(int month, int day, int year)
{
	auto ymd = std::chrono::year_month_day(std::chrono::year(year), std::chrono::month(month), std::chrono::day(day));

	return std::chrono::duration_cast<std::chrono::milliseconds>(static_cast<std::chrono::sys_days>(ymd).time_since_epoch());
}

// done with local time
inline DateTimeRange range_for_date(int month, int day, int year)
{
	auto ymd = std::chrono::year_month_day(std::chrono::year(year), std::chrono::month(month), std::chrono::day(day));

	auto days = static_cast<std::chrono::local_days>(ymd);

	DateTimeRange range;
	range.start = std::chrono::duration_cast<std::chrono::milliseconds>(days.time_since_epoch());

	days += std::chrono::days(1);
	range.end = std::chrono::duration_cast<std::chrono::milliseconds>(days.time_since_epoch());

	range.start -= std::chrono::current_zone()->get_info(std::chrono::system_clock::now()).offset;
	range.end -= std::chrono::current_zone()->get_info(std::chrono::system_clock::now()).offset;

	return range;
}

struct Clock
{
	virtual ~Clock() = default;

	virtual std::chrono::milliseconds now() const
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	}

	virtual std::chrono::milliseconds midnight() const
	{
		auto point = std::chrono::system_clock::now();
		auto days = std::chrono::floor<std::chrono::days>(point).time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(days);
	}

	std::chrono::milliseconds midnight(std::chrono::system_clock::time_point point) const
	{
		auto days = std::chrono::floor<std::chrono::days>(point).time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(days);
	}
};
