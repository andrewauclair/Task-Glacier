#pragma once

struct TimeCode
{
	TimeCodeID id; // the ID will be continuously incremented, even when deleting time codes that were just created
	std::string name;
	bool inUse = false;
	std::int32_t taskCount;
	bool archived = false;

	friend std::ostream& operator<<(std::ostream& out, const TimeCode& code)
	{
		out << "TimeCode { id: " << code.id._val << ", name: " << code.name << ", archived: " << code.archived << " }";

		return out;
	}

	constexpr auto operator<=>(const TimeCode&) const = default;
};
