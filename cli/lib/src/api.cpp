#include "api.hpp"

namespace
{
struct MessageProcessVisitor : MessageVisitor
{
	MicroTask& app;
	std::vector<std::unique_ptr<Message>>& output;

	MessageProcessVisitor(MicroTask& app, std::vector<std::unique_ptr<Message>>& output) : app(app), output(output) {}

	void visit(const CreateTaskMessage& message) override;
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
	const auto result = app.create_task(message.name, message.parentID);

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
		const auto send_task = [&](const Task& task)
		{
			output.push_back(std::make_unique<TaskInfoMessage>(task.taskID(), task.parentID(), task.m_name));
		};

		app.for_each_task_sorted(send_task);

		output.push_back(std::make_unique<EmptyMessage>(PacketType::REQUEST_CONFIGURATION_COMPLETE));
	}
}
