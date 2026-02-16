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

	constexpr bool operator==(const TimeCategory& other) const
	{
		return id == other.id;
	}

	constexpr bool operator!=(const TimeCategory& other) const
	{
		return id != other.id;
	}

	constexpr bool operator<(const TimeCategory& other) const
	{
		return id < other.id;
	}
};

struct TimeCategories
{
	std::vector<TimeCategory> categories;

	std::pair<TimeCategory, TimeCode> find(TimeCategoryID categoryID, TimeCodeID codeID) const
	{
		TimeCategory unknownCategory{ TimeCategoryID(0), "Unknown" };
		TimeCode unknownCode{ TimeCodeID(0), "Unknown" };

		for (auto&& category : categories)
		{
			if (category.id == categoryID)
			{
				for (auto&& code : category.codes)
				{
					if (code.id == codeID)
					{
						return { category, code };
					}
				}

				return { category, unknownCode };
			}
		}

		return { unknownCategory, unknownCode };
	}
};