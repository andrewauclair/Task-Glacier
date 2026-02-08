#pragma once

#include "time_code.hpp"
#include "time_category_id.hpp"

#include <string>
#include <ostream>
#include <vector>

struct TimeCategory
{
	TimeCategoryID id; // the ID will be continuously incremented, even when deleting time categories that were just created
	std::string name;
	std::vector<TimeCode> codes;

	friend std::ostream& operator<<(std::ostream& out, const TimeCategory& category)
	{
		out << "TimeCategory { id: " << category.id._val << ", name: " << category.name << ", \n";

		for (auto&& code : category.codes)
		{
			out << code << '\n';
		}

		out << " }";

		return out;
	}

	constexpr auto operator<=>(const TimeCategory&) const = default;
};
