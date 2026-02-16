#pragma once

#include "time_code_id.hpp"

struct TimeCode
{
	TimeCodeID id; // the ID will be continuously incremented, even when deleting time codes that were just created
	std::string name;

	bool archived = false;

	friend std::ostream& operator<<(std::ostream& out, const TimeCode& code)
	{
		out << "TimeCode { id: " << code.id._val << ", name: " << code.name << ", archived: " << code.archived << " }";

		return out;
	}

	constexpr bool operator==(const TimeCode& other) const
	{
		return id == other.id;
	}

	constexpr bool operator!=(const TimeCode& other) const
	{
		return id != other.id;
	}

	constexpr bool operator<(const TimeCode& other) const
	{
		return id < other.id;
	}
};
