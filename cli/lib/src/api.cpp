#include "api.hpp"

namespace
{
struct MessageProcessVisitor : MessageVisitor
{
	MicroTask& app;
	std::vector<std::unique_ptr<Message>>& output;

	MessageProcessVisitor(MicroTask& app, std::vector<std::unique_ptr<Message>>& output) : app(app), output(output) {}

	void visit(const CreateTaskMessage& message) override;
	void visit(const CreateListMessage& message) override;
	void visit(const CreateGroupMessage& message) override;
	void visit(const EmptyMessage& message) override;
};
}

void API::process_packet(const Message& message, std::vector<std::unique_ptr<Message>>& output)
{
	auto handler = MessageProcessVisitor(m_app, output);
	message.visit(handler);
}

void MessageProcessVisitor::visit(const CreateTaskMessage& message)
{
	const auto result = app.create_task(message.name, message.listID);

	if (result)
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));
	}
	else
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.error()));
	}
}

void MessageProcessVisitor::visit(const CreateListMessage& message)
{
	const auto result = app.create_list(message.name, message.groupID);

	if (result)
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));
	}
	else
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.error()));
	}
}

void MessageProcessVisitor::visit(const CreateGroupMessage& message)
{
	const auto result = app.create_group(message.name, message.groupID);

	if (result)
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));
	}
	else
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.error()));
	}
}

void MessageProcessVisitor::visit(const EmptyMessage& message)
{
	if (message.packetType == PacketType::REQUEST_CONFIGURATION)
	{
		const auto add_group = [&](auto add_group, const Group& group) -> void
			{
				output.push_back(std::make_unique<GroupInfoMessage>(group.groupID(), group.name()));

				for (auto&& g : group.m_groups)
				{
					add_group(add_group, g);
				}

				for (auto&& list : group.m_lists)
				{
					output.push_back(std::make_unique<ListInfoMessage>(group.groupID(), list.listID(), list.name()));

					for (auto&& task : list.m_tasks)
					{
						output.push_back(std::make_unique<TaskInfoMessage>(task.taskID(), list.listID(), task.m_name));
					}
				}
			};

		const Group& root = app.root();

		add_group(add_group, root);

		output.push_back(std::make_unique<EmptyMessage>(PacketType::REQUEST_CONFIGURATION_COMPLETE));
	}
}
