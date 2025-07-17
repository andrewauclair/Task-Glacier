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
	builder.add(name);
	builder.add(static_cast<std::int32_t>(labels.size()));
	for (auto&& label : labels)
	{
		builder.add(label);
	}
	builder.add(static_cast<std::int32_t>(timeEntry.size()));

	for (auto&& time : timeEntry)
	{
		builder.add(time.categoryID);
		builder.add(time.codeID);
	}
	
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
		auto task = CreateTaskMessage(parentID.value(), requestID.value(), name.value());

		const auto labelCount = parser.parse_next_immediate<std::int32_t>();

		for (std::int32_t i = 0; i < labelCount; i++)
		{
			task.labels.push_back(parser.parse_next<std::string>().value());
		}

		const auto timeCodeCount = parser.parse_next_immediate<std::int32_t>();

		for (int i = 0; i < timeCodeCount; i++)
		{
			auto category = parser.parse_next_immediate<TimeCategoryID>();
			auto code = parser.parse_next_immediate<TimeCodeID>();

			task.timeEntry.push_back(TimeEntry{ category, code });
		}

		return task;
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
	builder.add(indexInParent);
	builder.add(serverControlled);
	builder.add(locked);
	builder.add(name);
	builder.add(static_cast<std::int32_t>(labels.size()));
	for (auto&& label : labels)
	{
		builder.add(label);
	}
	builder.add(static_cast<std::int32_t>(timeEntry.size()));
	for (auto&& time : timeEntry)
	{
		builder.add(time.categoryID);
		builder.add(time.codeID);
	}
	return builder.build();
}

std::expected<UpdateTaskMessage, UnpackError> UpdateTaskMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto taskID = parser.parse_next<TaskID>();
	const auto parentID = parser.parse_next<TaskID>();
	const auto indexInParent = parser.parse_next<std::int32_t>();
	const auto serverControlled = parser.parse_next<bool>();
	const auto locked = parser.parse_next<bool>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		const auto labelCount = parser.parse_next_immediate<std::int32_t>();

		auto update = UpdateTaskMessage(requestID.value(), taskID.value(), parentID.value(), name.value());
		update.indexInParent = indexInParent.value();
		update.serverControlled = serverControlled.value();
		update.locked = locked.value();

		for (std::int32_t i = 0; i < labelCount; i++)
		{
			update.labels.push_back(parser.parse_next<std::string>().value());
		}

		const auto timeCodeCount = parser.parse_next_immediate<std::int32_t>();

		for (int i = 0; i < timeCodeCount; i++)
		{
			auto category = parser.parse_next_immediate<TimeCategoryID>();
			auto code = parser.parse_next_immediate<TimeCodeID>();

			update.timeEntry.push_back(TimeEntry{ category, code });
		}

		return update;
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

std::vector<std::byte> TimeEntryDataPacket::pack() const
{
	PacketBuilder builder;

	builder.add(packetType());

	builder.add(static_cast<std::int32_t>(timeCategories.size()));

	for (auto&& timeCategory : timeCategories)
	{
		builder.add(timeCategory.id);
		builder.add(timeCategory.name);
		builder.add(timeCategory.label);
		builder.add(timeCategory.inUse);
		builder.add(timeCategory.taskCount);
		builder.add(timeCategory.archived);
		builder.add(static_cast<std::int32_t>(timeCategory.codes.size()));

		for (auto&& timeCode : timeCategory.codes)
		{
			builder.add(timeCode.id);
			builder.add(timeCode.name);
			builder.add(timeCode.inUse);
			builder.add(timeCode.taskCount);
			builder.add(timeCode.archived);
		}
	}
	return builder.build();
}

std::expected<TimeEntryDataPacket, UnpackError> TimeEntryDataPacket::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	
	try
	{
		TimeEntryDataPacket data({});

		const std::int32_t timeCategoryCount = parser.parse_next<std::int32_t>().value();

		for (int i = 0; i < timeCategoryCount; i++)
		{
			auto id = parser.parse_next_immediate<TimeCategoryID>();
			auto name = parser.parse_next_immediate<std::string>();
			auto label = parser.parse_next_immediate<std::string>();
			TimeCategory timeCategory(id, name);
			timeCategory.label = label;
			timeCategory.inUse = parser.parse_next_immediate<bool>();
			timeCategory.taskCount = parser.parse_next_immediate<std::int32_t>();
			timeCategory.archived = parser.parse_next_immediate<bool>();

			const std::int32_t timeCodeCount = parser.parse_next_immediate<std::int32_t>();

			for (int j = 0; j < timeCodeCount; j++)
			{
				auto codeID = parser.parse_next_immediate<TimeCodeID>();
				auto codeName = parser.parse_next_immediate<std::string>();
				TimeCode timeCode(codeID, codeName);
				timeCode.inUse = parser.parse_next_immediate<bool>();
				timeCode.taskCount = parser.parse_next_immediate<std::int32_t>();
				timeCode.archived = parser.parse_next_immediate<bool>();

				timeCategory.codes.push_back(timeCode);
			}
			data.timeCategories.push_back(timeCategory);
		}
		
		return data;
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> TimeEntryModifyPacket::pack() const
{
	PacketBuilder builder;

	builder.add(static_cast<std::int32_t>(packetType()));
	builder.add(requestID);
	builder.add(type);
	builder.add<std::int32_t>(timeCategories.size());

	for (auto&& category : timeCategories)
	{
		builder.add(category.id);
		builder.add(category.name);
		builder.add(category.label);
		builder.add(category.archived);

		builder.add<std::int32_t>(category.codes.size());

		for (auto&& code : category.codes)
		{
			builder.add(code.id);
			builder.add(code.name);
			builder.add(code.archived);
		}
	}
	
	return builder.build();
}

std::expected<TimeEntryModifyPacket, UnpackError> TimeEntryModifyPacket::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto modType = parser.parse_next<TimeCategoryModType>();

	const auto categoryCount = parser.parse_next<std::int32_t>();

	try
	{
		std::vector<TimeCategory> categories;

		for (int i = 0; i < categoryCount.value(); i++)
		{
			TimeCategory category(parser.parse_next_immediate<TimeCategoryID>());

			category.name = parser.parse_next_immediate<std::string>();
			category.label = parser.parse_next_immediate<std::string>();
			category.archived = parser.parse_next_immediate<bool>();

			const auto codeCount = parser.parse_next_immediate<std::int32_t>();
			for (int j = 0; j < codeCount; j++)
			{
				const auto id = parser.parse_next<TimeCodeID>();
				const auto name = parser.parse_next<std::string>();
				const auto archived = parser.parse_next<bool>();

				category.codes.emplace_back(id.value(), name.value(), archived.value());
			}
			categories.push_back(category);
		}
		return TimeEntryModifyPacket(requestID.value(), modType.value(), categories);
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
	builder.add(message);

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
	builder.add(serverControlled);
	builder.add(locked);
	builder.add(name);
	builder.add(createTime);
	builder.add(finishTime.has_value());
	builder.add(finishTime.value_or(std::chrono::milliseconds(0)));

	builder.add(static_cast<std::int32_t>(times.size()));

	for (auto&& time : times)
	{
		builder.add(time.start);
		builder.add(time.stop.has_value());
		builder.add(time.stop.value_or(std::chrono::milliseconds(0)));

		builder.add(static_cast<std::int32_t>(time.timeEntry.size()));

		for (auto&& entry : time.timeEntry)
		{
			builder.add(entry.categoryID);
			builder.add(entry.codeID);
		}
	}

	builder.add(static_cast<std::int32_t>(labels.size()));
	
	for (const std::string& label : labels)
	{
		builder.add(label);
	}

	builder.add(static_cast<std::int32_t>(timeEntry.size()));

	for (auto&& time : timeEntry)
	{
		builder.add(time.categoryID);
		builder.add(time.codeID);
	}

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
	const auto serverControlled = parser.parse_next<bool>();
	const auto locked = parser.parse_next<bool>();
	const auto name = parser.parse_next<std::string>();

	try
	{
		auto info = TaskInfoMessage(taskID.value(), parentID.value(), name.value());
		info.state = state.value();
		info.newTask = newTask.value();
		info.serverControlled = serverControlled.value();
		info.locked = locked.value();

		info.createTime = parser.parse_next_immediate<std::chrono::milliseconds>();

		const bool finishTimePresent = parser.parse_next_immediate<bool>();

		if (finishTimePresent)
		{
			info.finishTime = parser.parse_next_immediate<std::chrono::milliseconds>();
		}

		const auto startStopCount = parser.parse_next_immediate<std::int32_t>();

		for (std::int32_t i = 0; i < startStopCount; i++)
		{
			TaskTimes times;

			times.start = parser.parse_next_immediate<std::chrono::milliseconds>();

			const bool stopPresent = parser.parse_next_immediate<bool>();

			if (stopPresent)
			{
				times.stop = parser.parse_next_immediate<std::chrono::milliseconds>();
			}

			const auto entryCount = parser.parse_next_immediate<std::int32_t>();

			for (std::int32_t j = 0; j < entryCount; j++)
			{
				auto category = parser.parse_next_immediate<TimeCategoryID>();
				auto code = parser.parse_next_immediate<TimeCodeID>();

				times.timeEntry.emplace_back(category, code);
			}
			info.times.push_back(times);
		}

		const auto labelCount = parser.parse_next_immediate<std::int32_t>();

		for (std::int32_t i = 0; i < labelCount; i++)
		{
			info.labels.push_back(parser.parse_next_immediate<std::string>());
		}

		const auto codeCount = parser.parse_next_immediate<std::int32_t>();

		for (std::int32_t i = 0; i < codeCount; i++)
		{
			auto category = parser.parse_next_immediate<TimeCategoryID>();
			auto code = parser.parse_next_immediate<TimeCodeID>();

			info.timeEntry.push_back(TimeEntry{ category, code });
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
	builder.add(instanceID);
	builder.add(name);
	builder.add(URL);
	builder.add(apiKey);
	builder.add(username);
	builder.add(rootTaskID);

	builder.add<std::int32_t>(groupTasksBy.size());

	for (auto&& value : groupTasksBy)
	{
		builder.add(value);
	}
	
	builder.add<std::int32_t>(labelToField.size());

	for (auto&& value : labelToField)
	{
		builder.add(value.first);
		builder.add(value.second);
	}
	return builder.build();
}

std::expected<BugzillaInfoMessage, UnpackError> BugzillaInfoMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto id = parser.parse_next<BugzillaInstanceID>();
	const auto name = parser.parse_next<std::string>();
	const auto URL = parser.parse_next<std::string>();
	const auto apiKey = parser.parse_next<std::string>();
	const auto username = parser.parse_next<std::string>();
	const auto rootTaskID = parser.parse_next<TaskID>();

	try
	{
		auto info = BugzillaInfoMessage(id.value(), name.value(), URL.value(), apiKey.value());
		info.username = username.value();
		info.rootTaskID = rootTaskID.value();

		const std::int32_t groupByCount = parser.parse_next_immediate<std::int32_t>();

		for (int i = 0; i < groupByCount; i++)
		{
			info.groupTasksBy.push_back(parser.parse_next_immediate<std::string>());
		}

		const std::int32_t count = parser.parse_next_immediate<std::int32_t>();

		for (int i = 0; i < count; i++)
		{
			const auto label = parser.parse_next_immediate<std::string>();
			const auto field = parser.parse_next_immediate<std::string>();

			info.labelToField.emplace(label, field);
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

std::vector<std::byte> RequestWeeklyReportMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::REQUEST_WEEKLY_REPORT);
	builder.add(requestID);
	builder.add<std::int8_t>(month);
	builder.add<std::int8_t>(day);
	builder.add<std::int16_t>(year);

	return builder.build();
}

std::expected<RequestWeeklyReportMessage, UnpackError> RequestWeeklyReportMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();
	const auto month = parser.parse_next<std::int8_t>();
	const auto day = parser.parse_next<std::int8_t>();
	const auto year = parser.parse_next<std::int16_t>();

	try
	{
		return RequestWeeklyReportMessage(requestID.value(), month.value(), day.value(), year.value());
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
	builder.add(report.found);
	builder.add<std::int8_t>(report.month);
	builder.add<std::int8_t>(report.day);
	builder.add<std::int16_t>(report.year);
	
	if (report.found)
	{
		builder.add(report.startTime);
		builder.add(report.endTime);
		builder.add(report.totalTime);

		builder.add<std::int32_t>(report.timePerTimeEntry.size());

		for (auto&& timeEntry : report.timePerTimeEntry)
		{
			builder.add(timeEntry.first.categoryID);
			builder.add(timeEntry.first.codeID);
			builder.add(timeEntry.second);
		}

		builder.add<std::int32_t>(report.times.size());

		for (auto&& pair : report.times)
		{
			builder.add(pair.taskID);
			builder.add(pair.startStopIndex);
		}
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

		auto report = DailyReportMessage(requestID.value());

		report.report.found = true;
		report.report.month = parser.parse_next_immediate<std::int8_t>();
		report.report.day = parser.parse_next_immediate<std::int8_t>();
		report.report.year = parser.parse_next_immediate<std::int16_t>();

		return report;
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}

std::vector<std::byte> WeeklyReportMessage::pack() const
{
	PacketBuilder builder;

	builder.add(PacketType::WEEKLY_REPORT);
	builder.add(requestID);

	for (auto&& report : dailyReports)
	{
		builder.add(report.found);
		builder.add<std::int8_t>(report.month);
		builder.add<std::int8_t>(report.day);
		builder.add<std::int16_t>(report.year);

		if (report.found)
		{
			builder.add(report.startTime);
			builder.add(report.endTime);
			builder.add(report.totalTime);

			builder.add<std::int32_t>(report.timePerTimeEntry.size());

			for (auto&& timeEntry : report.timePerTimeEntry)
			{
				builder.add(timeEntry.first.categoryID);
				builder.add(timeEntry.first.codeID);
				builder.add(timeEntry.second);
			}

			builder.add<std::int32_t>(report.times.size());

			for (auto&& pair : report.times)
			{
				builder.add(pair.taskID);
				builder.add(pair.startStopIndex);
			}
		}
	}

	return builder.build();
}

std::expected<WeeklyReportMessage, UnpackError> WeeklyReportMessage::unpack(std::span<const std::byte> data)
{
	auto parser = PacketParser(data);

	const auto packetType = parser.parse_next<PacketType>();
	const auto requestID = parser.parse_next<RequestID>();

	try
	{
		return WeeklyReportMessage(requestID.value());
	}
	catch (const std::bad_expected_access<UnpackError>& e)
	{
		return std::unexpected(e.error());
	}
}
