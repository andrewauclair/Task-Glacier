#pragma once

#include "time_category.hpp"
#include "time_code.hpp"

#include <ostream>

struct TimeEntry
{
	TimeCategory category;
	TimeCode code;

	constexpr bool operator==(const TimeEntry&) const = default;
	constexpr bool operator!=(const TimeEntry&) const = default;
	constexpr bool operator<(const TimeEntry& other) const
	{
		return std::tie(category.id, code.id) < std::tie(other.category.id, other.code.id);
	}

	friend std::ostream& operator<<(std::ostream& out, const TimeEntry& entry)
	{
		out << "TimeEntry { cat: " << entry.category.name << " (" << entry.category.id._val << "), code: " << entry.code.name << " (" << entry.code.id._val << ")";
		out << " }";

		return out;
	}
};
