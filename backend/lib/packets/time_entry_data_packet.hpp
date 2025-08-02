#pragma once

#include "message.hpp"
#include "time_category.hpp"

#include <ostream>
#include <vector>

struct TimeEntryDataPacket : Message
{
	std::vector<TimeCategory> timeCategories;

	TimeEntryDataPacket(std::vector<TimeCategory> timeCategories) : Message(PacketType::TIME_ENTRY_DATA), timeCategories(std::move(timeCategories))
	{
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TimeEntryDataPacket, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TimeEntryDataPacket { ";
		Message::print(out);

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

	friend std::ostream& operator<<(std::ostream& out, const TimeEntryDataPacket& message)
	{
		message.print(out);
		return out;
	}
};
