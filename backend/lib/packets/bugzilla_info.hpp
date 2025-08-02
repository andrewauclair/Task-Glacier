#pragma once

#include "message.hpp"

#include "bugzilla_instance_id.hpp"
#include "task_id.hpp"

#include <string>
#include <ostream>
#include <vector>

struct BugzillaInfoMessage : Message
{
	BugzillaInstanceID instanceID;
	std::string name;
	std::string URL;
	std::string apiKey;
	std::string username;
	TaskID rootTaskID = NO_PARENT;
	std::vector<std::string> groupTasksBy;
	std::map<std::string, std::string> labelToField;

	BugzillaInfoMessage(BugzillaInstanceID instanceID, std::string name, std::string URL, std::string apiKey) : Message(PacketType::BUGZILLA_INFO), instanceID(instanceID), name(std::move(name)), URL(std::move(URL)), apiKey(std::move(apiKey)) {}

	std::vector<std::byte> pack() const override;
	static std::expected<BugzillaInfoMessage, UnpackError> unpack(std::span<const std::byte> data);

	std::ostream& print(std::ostream& out) const override
	{
		out << "BugzillaInfoMessage { id: " << instanceID._val << ", name: " << name << ", URL : \"" << URL << "\", apiKey: \"" << apiKey << "\", username: " << username << ", rootTaskID: " << rootTaskID._val << " }";

		out << '\n';
		out << "groupBy {\n";
		for (auto&& groupBy : groupTasksBy)
		{
			out << groupBy << '\n';
		}
		out << "}\n";
		out << "labelToField: {\n";
		for (auto&& [label, field] : labelToField)
		{
			out << label << ' ' << field << '\n';
		}
		out << "}\n";
		return out;
	}

	friend std::ostream& operator<<(std::ostream& out, const BugzillaInfoMessage& message)
	{
		return message.print(out);
	}
};
