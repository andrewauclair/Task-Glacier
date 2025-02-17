#include "packets.hpp"

std::vector<std::byte> RequestMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(packetType()));
	builder.add(requestID);

	return builder.build();
}

std::expected<RequestMessage, UnpackError> RequestMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();

	try
	{
		return RequestMessage(packetType.value(), requestID.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> CreateTaskMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::CREATE_TASK));
	builder.add(requestID);
	builder.add(parentID);
	builder.add_string(name);

	return builder.build();
}

std::expected<CreateTaskMessage, UnpackError> CreateTaskMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);
	
	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto parentID = parser.parse_next<TaskID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return CreateTaskMessage(parentID.value(), requestID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> UpdateTaskMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(packetType()));
	builder.add(requestID);
	builder.add(taskID);
	builder.add(parentID);
	builder.add_string(name);

	return builder.build();
}

std::expected<UpdateTaskMessage, UnpackError> UpdateTaskMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto taskID = parser.parse_next<TaskID>();
	const auto parentID = parser.parse_next<TaskID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return UpdateTaskMessage(requestID.value(), taskID.value(), parentID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> TaskMessage::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(packetType()));
	builder.add(requestID);
	builder.add(taskID);

	return builder.build();
}

std::expected<TaskMessage, UnpackError> TaskMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto taskID = parser.parse_next<TaskID>();

	try
	{
		return TaskMessage(packetType.value(), requestID.value(), taskID.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> SuccessResponse::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(PacketType::SUCCESS_RESPONSE));
	builder.add(requestID);

	return builder.build();
}

std::expected<SuccessResponse, UnpackError> SuccessResponse::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();

	if (requestID)
	{
		return SuccessResponse(requestID.value());
	}
	return std::unexpected(requestID.error());
}

std::vector<std::byte> FailureResponse::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::FAILURE_RESPONSE);
	builder.add(requestID);
	builder.add_string(message);

	return builder.build();
}

std::expected<FailureResponse, UnpackError> FailureResponse::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		return FailureResponse(requestID.value(), name.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> BasicMessage::pack() const
{
	PacketBuilder builder;

	builder.add(packetType());

	return builder.build();
}

std::expected<BasicMessage, UnpackError> BasicMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();

	if (packetType)
	{
		return BasicMessage(packetType.value());
	}
	return std::unexpected(packetType.error());
}

std::vector<std::byte> TaskInfoMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::TASK_INFO);
	builder.add(taskID);
	builder.add(parentID);
	builder.add(state);
	builder.add(newTask);
	builder.add_string(name);
	builder.add(createTime);

	builder.add(static_cast<std::int32_t>(times.size()));

	for (auto&& time : times)
	{
		builder.add(time.start);
		builder.add(time.stop.has_value());
		builder.add(time.stop.value_or(std::chrono::milliseconds(0)));
	}

	builder.add(finishTime.has_value());
	builder.add(finishTime.value_or(std::chrono::milliseconds(0)));

	return builder.build();
}

std::expected<TaskInfoMessage, UnpackError> TaskInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto taskID = parser.parse_next<TaskID>();
	const auto parentID = parser.parse_next<TaskID>();
	const auto state = parser.parse_next<TaskState>();
	const auto newTask = parser.parse_next<bool>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		auto info = TaskInfoMessage(taskID.value(), parentID.value(), name.value());
		info.state = state.value();
		info.newTask = newTask.value();

		info.createTime = parser.parse_next<std::chrono::milliseconds>().value();

		const auto startStopCount = parser.parse_next<std::int32_t>().value();

		for (std::int32_t i = 0; i < startStopCount; i++)
		{
			TaskTimes times;

			times.start = parser.parse_next<std::chrono::milliseconds>().value();

			const bool stopPresent = parser.parse_next<bool>().value();

			if (stopPresent)
			{
				times.stop = parser.parse_next<std::chrono::milliseconds>().value();
			}

			info.times.push_back(times);
		}

		const bool finishTimePresent = parser.parse_next<bool>().value();
		
		if (finishTimePresent)
		{
			info.finishTime = parser.parse_next<std::chrono::milliseconds>().value();
		}

		return info;
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> BugzillaInfoMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::BUGZILLA_INFO);
	builder.add_string(URL);
	builder.add_string(apiKey);
	builder.add_string(username);
	builder.add(rootTaskID);
	builder.add_string(groupTasksBy);
	
	builder.add<std::int32_t>(labelToField.size());

	for (auto&& value : labelToField)
	{
		builder.add_string(value.first);
		builder.add_string(value.second);
	}
	return builder.build();
}

std::expected<BugzillaInfoMessage, UnpackError> BugzillaInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto URL = parser.parse_next<std::string>();
	const auto apiKey = parser.parse_next<std::string>();
	const auto username = parser.parse_next<std::string>();
	const auto rootTaskID = parser.parse_next<TaskID>();
	const auto groupTasksBy = parser.parse_next<std::string>();

	try
	{
		auto info = BugzillaInfoMessage(URL.value(), apiKey.value());
		info.username = username.value();
		info.rootTaskID = rootTaskID.value();
		info.groupTasksBy = groupTasksBy.value();

		const std::int32_t count = parser.parse_next<std::int32_t>().value();

		for (int i = 0; i < count; i++)
		{
			const auto label = parser.parse_next<std::string>();
			const auto field = parser.parse_next<std::string>();

			info.labelToField.emplace(label.value(), field.value());
		}

		return info;
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> RequestDailyReportMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::REQUEST_DAILY_REPORT);
	builder.add(requestID);
	builder.add<std::int8_t>(month);
	builder.add<std::int8_t>(day);
	builder.add<std::int16_t>(year);

	return builder.build();
}

std::expected<RequestDailyReportMessage, UnpackError> RequestDailyReportMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto month = parser.parse_next<std::int8_t>();
	const auto day = parser.parse_next<std::int8_t>();
	const auto year = parser.parse_next<std::int16_t>();

	try
	{
		return RequestDailyReportMessage(requestID.value(), month.value(), day.value(), year.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> DailyReportMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::DAILY_REPORT);
	builder.add(requestID);
	builder.add(reportFound);
	
	if (reportFound)
	{
		builder.add<std::int8_t>(report.month);
		builder.add<std::int8_t>(report.day);
		builder.add<std::int16_t>(report.year);
	}

	return builder.build();
}

std::expected<DailyReportMessage, UnpackError> DailyReportMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto reportFound = parser.parse_next<bool>();
	

	try
	{
		if (!reportFound.value())
		{
			return DailyReportMessage(requestID.value());
		}

		const auto month = parser.parse_next<std::int8_t>();
		const auto day = parser.parse_next<std::int8_t>();
		const auto year = parser.parse_next<std::int16_t>();

		auto report = DailyReportMessage(requestID.value());

		report.reportFound = true;
		report.report.month = month.value();
		report.report.day = day.value();
		report.report.year = year.value();

		return report;
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}
