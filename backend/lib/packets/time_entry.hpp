#pragma once

#include "time_category_id.hpp"
#include "time_code_id.hpp"

#include <ostream>

struct TimeEntry
{
	TimeCategoryID categoryID;
	TimeCodeID codeID;

	constexpr auto operator<=>(const TimeEntry&) const = default;

	friend std::ostream& operator<<(std::ostream& out, const TimeEntry& entry)
	{
		out << "TimeEntry { catID: " << entry.categoryID._val << "codeID: " << entry.codeID._val;
		out << " }";

		return out;
	}
};
