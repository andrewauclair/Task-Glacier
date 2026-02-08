#pragma once

#include "request.hpp"
#include "time_category_mod_type.hpp"
#include "time_category.hpp"

#include <ostream>
#include <vector>

struct TimeEntryModifyPacket : RequestMessage
{
	struct Category
	{
		TimeCategoryModType type;
		TimeCategoryID id;
		std::string name;
	};
	struct Code
	{
		TimeCategoryModType type;
		std::int32_t categoryIndex;
		TimeCodeID codeID;
		std::string name;
		bool archive;
	};

	std::vector<Category> categories;
	std::vector<Code> codes;

	TimeEntryModifyPacket(RequestID requestID) : RequestMessage(PacketType::TIME_ENTRY_MODIFY, requestID)
	{
	}

	std::vector<std::byte> pack() const override;
	static std::expected<TimeEntryModifyPacket, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "TimeEntryModifyPacket { ";
		RequestMessage::print(out);

		if (!categories.empty())
		{
			out << ", {\n";

			for (const Category& category : categories)
			{
				out << "    type: " << static_cast<int>(category.type) << ", id: " << category.id._val << ", name: " << category.name << '\n';
			}
			out << "}\n";
		}

		if (!codes.empty())
		{
			out << ", {\n";

			for (const Code& code : codes)
			{
				out << "    type: " << static_cast<int>(code.type) << ", cat index: " << code.categoryIndex << ", code id: " << code.codeID._val << ", name: " << code.name << ", archive: " << code.archive << '\n';
			}
			out << "}\n";
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
