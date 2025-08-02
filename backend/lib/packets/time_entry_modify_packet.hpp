#pragma once

#include "request.hpp"
#include "time_category_mod_type.hpp"
#include "time_category.hpp"

#include <ostream>
#include <vector>

struct TimeEntryModifyPacket : RequestMessage
{
	TimeCategoryModType type;
	std::vector<TimeCategory> timeCategories;

	TimeEntryModifyPacket(RequestID requestID, TimeCategoryModType type, std::vector<TimeCategory> timeCategories) : RequestMessage(PacketType::TIME_ENTRY_MODIFY, requestID), type(type), timeCategories(std::move(timeCategories))
	{
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TimeEntryModifyPacket, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TimeEntryModifyPacket { ";
		RequestMessage::print(out);

		out << ", type: " << static_cast<int>(type);

		if (!timeCategories.empty())
		{
			out << ", ";
		}

		for (auto&& category : timeCategories)
		{
			out << category;
		}
		out << " }";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const TimeEntryModifyPacket& message)
	{
		message.print(out);
		return out;
	}
};
