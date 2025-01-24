#include "api.hpp"

namespace
{
struct MessageProcessVisitor : MessageVisitor
{
	MicroTask& app;
	std::vector<std::unique_ptr<Message>>& output;

	MessageProcessVisitor(MicroTask& app, std::vector<std::unique_ptr<Message>>& output) : app(app), output(output) {}

	void visit(const CreateTaskMessage& message) override;
	void visit(const StartTaskMessage& message) override;
	void visit(const StopTaskMessage& message) override;
	void visit(const FinishTaskMessage& message) override;
	void visit(const BasicMessage& message) override;
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

		auto* task = app.find_task(result.value());

		TaskInfoMessage info(task->taskID(), task->parentID(), task->m_name);
		info.newTask = true;
		info.state = task->state;
		info.createTime = task->createTime();

		output.push_back(std::make_unique<TaskInfoMessage>(info));
	}
	else
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.error()));
	}
}

void MessageProcessVisitor::visit(const StartTaskMessage& message)
{
	const auto result = app.start_task(message.taskID);

	if (result)
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = app.find_task(message.taskID);

		TaskInfoMessage info(task->taskID(), task->parentID(), task->m_name);
		info.state = task->state;
		info.createTime = task->createTime();
		info.times.insert(info.times.end(), task->times().begin(), task->times().end());

		output.push_back(std::make_unique<TaskInfoMessage>(info));
	}
}

void MessageProcessVisitor::visit(const StopTaskMessage& message)
{
	const auto result = app.stop_task(message.taskID);

	if (result)
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = app.find_task(message.taskID);

		TaskInfoMessage info(task->taskID(), task->parentID(), task->m_name);
		info.state = task->state;
		info.createTime = task->createTime();
		info.times.insert(info.times.end(), task->times().begin(), task->times().end());

		output.push_back(std::make_unique<TaskInfoMessage>(info));
	}
}

void MessageProcessVisitor::visit(const FinishTaskMessage& message)
{
	const auto result = app.finish_task(message.taskID);

	if (result)
	{
		output.push_back(std::make_unique<FailureResponse>(message.requestID, result.value()));
	}
	else
	{
		output.push_back(std::make_unique<SuccessResponse>(message.requestID));

		auto* task = app.find_task(message.taskID);

		TaskInfoMessage info(task->taskID(), task->parentID(), task->m_name);
		info.state = task->state;
		info.createTime = task->createTime();
		info.times.insert(info.times.end(), task->times().begin(), task->times().end());

		output.push_back(std::make_unique<TaskInfoMessage>(info));
	}
}

void MessageProcessVisitor::visit(const BasicMessage& message)
{
	if (message.packetType == PacketType::REQUEST_CONFIGURATION)
	{
		const auto send_task = [&](const Task& task)
		{
			auto info = std::make_unique<TaskInfoMessage>(task.taskID(), task.parentID(), task.m_name);
			info->createTime = task.createTime();
			info->finishTime = task.finishTime();
			auto times = task.times();
			info->times = std::vector<TaskTimes>(times.begin(), times.end());

			output.push_back(std::move(info));
		};

		app.for_each_task_sorted(send_task);

		output.push_back(std::make_unique<BasicMessage>(PacketType::REQUEST_CONFIGURATION_COMPLETE));
	}
}
