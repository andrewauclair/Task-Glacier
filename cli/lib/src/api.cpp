#include "api.hpp"

API::API(std::vector<MessageTypes>& output) : m_output(output)
{
}

// helper type for the visitor
template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

void API::process_packet(const MessageTypes& message)
{
	const auto handle_create_list = [&](const CreateListMessage& message)
		{
			const auto result = m_app.create_list(message.name, message.groupID);

			if (result)
			{
				m_output.push_back(SuccessResponse{ message.requestID });
			}
			else
			{
				m_output.push_back(FailureResponse{ result.error() });
			}
		};
	const auto handle_create_group = [](const CreateGroupMessage&) {};
	const auto handle_success = [](const SuccessResponse&) {};
	const auto handle_failure = [](const FailureResponse&) {};

	std::visit(overloads{ handle_create_list, handle_create_group, handle_success, handle_failure }, message);
}
