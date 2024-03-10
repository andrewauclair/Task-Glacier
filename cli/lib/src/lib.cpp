#include "lib.h"

#include <format>

List::List(std::string name, ListID id) : m_name(std::move(name)), m_listID(id) {}

Group::Group(std::string name, GroupID id) : m_name(std::move(name)), m_groupID(id) {}

std::expected<TaskID, std::string> MicroTask::create_task(const std::string& name, ListID listID)
{
	auto* list = find_list_by_id(listID);

	if (list)
	{
		auto id = m_nextTaskID;

		m_nextTaskID++;

		return id;
	}

	return std::unexpected(std::format("List with ID {} does not exist.", listID));
}

std::expected<ListID, std::string> MicroTask::create_list(const std::string& name, GroupID groupID)
{
	Group* group = find_group_by_id(groupID);

	if (find_list_by_name(name, group) != nullptr)
	{
		return std::unexpected(std::format("List with name '{}' already exists in group with ID {}.", name, groupID));
	}

	if (group)
	{
		List& new_list = group->m_lists.emplace_back(name, m_nextListID);

		m_nextListID++;

		return new_list.listID();
	}

	return std::unexpected(std::format("Group with ID {} does not exist.", groupID));
}

std::expected<GroupID, std::string> MicroTask::create_group(const std::string& name, GroupID groupID)
{
	Group* group = groupID == ROOT_GROUP_ID ? &m_root : find_group_by_id(groupID);

	if (find_group_by_name(name, group) != nullptr)
	{
		return std::unexpected(std::format("Group with name '{}' already exists in group with ID {}.", name, groupID));
	}

	if (group)
	{
		Group& new_group = group->m_groups.emplace_back(name, m_nextGroupID);

		m_nextGroupID++;

		return new_group.groupID();
	}

	return std::unexpected(std::format("Group with ID {} does not exist.", groupID));
}

std::optional<std::string> MicroTask::move_list(ListID listID, GroupID targetGroupID)
{
	//auto* list = find_list_by_id(listID);
	//auto* currentGroup = find_group_by_id(list->)
	//auto* targetGroup = find_group_by_id(targetGroupID);



	return std::nullopt;
}

std::optional<std::string> MicroTask::move_group(GroupID groupID, GroupID targetGroupID)
{
	return std::nullopt;
}

List* MicroTask::find_list_by_id(ListID listID)
{
	const auto search = [](auto search, Group& group, ListID listID) -> List*
		{
			for (auto&& otherGroup : group.m_groups)
			{
				List* result = search(search, otherGroup, listID);

				if (result)
				{
					return result;
				}
			}
			for (auto&& list : group.m_lists)
			{
				if (list.listID() == listID)
				{
					return &list;
				}
			}
			return nullptr;
		};

	return search(search, m_root, listID);
}

Group* MicroTask::find_group_by_id(GroupID groupID)
{
	const auto search = [](auto search, Group& group, GroupID groupID) -> Group*
		{
			for (auto&& otherGroup : group.m_groups)
			{
				if (otherGroup.groupID() == groupID)
				{
					return &otherGroup;
				}
				
				Group* result = search(search, otherGroup, groupID);

				if (result)
				{
					return result;
				}
			}
			return nullptr;
		};
	
	if (groupID == ROOT_GROUP_ID)
	{
		return &m_root;
	}
	return search(search, m_root, groupID);
}

List* MicroTask::find_list_by_name(std::string_view name, Group* group)
{
	if (group == nullptr)
	{
		group = &m_root;
	}

	for (auto&& otherList : group->m_lists)
	{
		if (otherList.name() == name)
		{
			return &otherList;
		}
	}
	return nullptr;
}

Group* MicroTask::find_group_by_name(std::string_view name, Group* group)
{
	if (group == nullptr)
	{
		group = &m_root;
	}

	for (auto&& otherGroup : group->m_groups)
	{
		if (otherGroup.name() == name)
		{
			return &otherGroup;
		}
	}
	return nullptr;
}
