#include "api.hpp"

namespace
{
struct MessageProcessVisitor : MessageVisitor
{
	MicroTask& app;
	std::vector<MessageTypes>& output;

	MessageProcessVisitor(MicroTask& app, std::vector<MessageTypes>& output) : app(app), output(output) {}

	void visit(const CreateListMessage& message) override;
	void visit(const CreateGroupMessage& message) override;
	void visit(const SuccessResponse& message) override;
	void visit(const FailureResponse& message) override;
};
}

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

void API::process_packet(const Message& message, std::vector<MessageTypes>& output)
{
	//std::visit(overloads{ handle_create_list, handle_create_group, handle_success, handle_failure }, message);
	auto handler = MessageProcessVisitor(m_app, output);
	message.visit(handler);
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

void MessageProcessVisitor::visit(const CreateGroupMessage& message){}

// these will never be received by the backend
void MessageProcessVisitor::visit(const SuccessResponse& message){}
void MessageProcessVisitor::visit(const FailureResponse& message){}
