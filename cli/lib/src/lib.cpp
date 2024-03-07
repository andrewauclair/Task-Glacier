#include "lib.h"

#include <format>

std::expected<ListID, std::string> MicroTask::create_list(const std::string& name, GroupID groupID)
{
	if (groupID == 0)
	{
		return 1;
	}

	Group* group = find_group(groupID);

	if (group)
	{
		return 1;
	}
	return std::unexpected(std::format("Group with ID {} does not exist.", groupID));
}

std::expected<GroupID, std::string> MicroTask::create_group(const std::string& name, GroupID groupID)
{
	if (groupID == 0)
	{
		return 1;
	}

	Group* group = find_group(groupID);

	if (group)
	{
		return 1;
	}

	return std::unexpected(std::format("Group with ID {} does not exist.", groupID));
}

List* MicroTask::find_list(ListID listID)
{
	return nullptr;
}

Group* MicroTask::find_group(GroupID groupID)
{
	const auto search_group = [](auto search_group, const Group& group, GroupID groupID)
		{
			for (auto&& otherGroup : group.m_groups)
			{
				if (otherGroup.id == groupID)
				{
					return &otherGroup;
				}
				
				Group* result = search_group(search_group, otherGroup, groupID);

				if (result)
				{
					return result;
				}
			}
			return nullptr;
		};
	Group* group = &m_root;


	return nullptr;
}
