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
