#pragma once

#include <cpptrace/cpptrace.hpp>

inline void verify_version_message(const VersionMessage& expected, const VersionMessage& actual, std::source_location location)
{
	CHECK(expected.version == actual.version);
}

inline void verify_create_task(const CreateTaskMessage& expected, const CreateTaskMessage& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.parentID == actual.parentID);
	CHECK(expected.name == actual.name);
	CHECK(expected.labels == actual.labels);
	CHECK(expected.timeEntry == actual.timeEntry);
}

inline void verify_task_message(const TaskMessage& expected, const TaskMessage& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.taskID == actual.taskID);
}

inline void verify_update_task(const UpdateTaskMessage& expected, const UpdateTaskMessage& actual, std::source_location location)
{
	CHECK(expected.taskID == actual.taskID);
	CHECK(expected.parentID == actual.parentID);
	CHECK(expected.indexInParent == actual.indexInParent);
	CHECK(expected.serverControlled == actual.serverControlled);
	CHECK(expected.locked == actual.locked);
	CHECK(expected.name == actual.name);
	CHECK(expected.times == actual.times);
	CHECK(expected.labels == actual.labels);
	CHECK(expected.timeEntry == actual.timeEntry);
}

inline void verify_success_response(const SuccessResponse& expected, const SuccessResponse& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
}

inline void verify_failure_response(const FailureResponse& expected, const FailureResponse& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.message == actual.message);
}

inline void verify_task_info(const TaskInfoMessage& expected, const TaskInfoMessage& actual, std::source_location location)
{
	CHECK(expected.taskID._val == actual.taskID._val);
	CHECK(expected.parentID._val == actual.parentID._val);
	CHECK(expected.state == actual.state);
	CHECK(expected.newTask == actual.newTask);
	CHECK(expected.indexInParent == actual.indexInParent);
	CHECK(expected.serverControlled == actual.serverControlled);
	CHECK(expected.locked == actual.locked);
	CHECK(expected.name == actual.name);
	CHECK(expected.createTime == actual.createTime);

	CHECK(expected.finishTime.has_value() == actual.finishTime.has_value());

	if (expected.finishTime.has_value() && actual.finishTime.has_value())
	{
		CHECK(expected.finishTime.value() == actual.finishTime.value());
	}

	CHECK(expected.times == actual.times);
	CHECK(expected.labels == actual.labels);
	CHECK(expected.timeEntry == actual.timeEntry);
}

inline void verify_bugzilla_info(const BugzillaInfoMessage& expected, const BugzillaInfoMessage& actual, std::source_location location)
{
	CHECK(expected.instanceID == actual.instanceID);
	CHECK(expected.name == actual.name);
	CHECK(expected.URL == actual.URL);
	CHECK(expected.apiKey == actual.apiKey);
	CHECK(expected.username == actual.username);
	CHECK(expected.rootTaskID == actual.rootTaskID);
	CHECK(expected.groupTasksBy == actual.groupTasksBy);
	CHECK(expected.labelToField == actual.labelToField);
}

inline void verify_daily_report(const DailyReportMessage& expected, const DailyReportMessage& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.reportTime == actual.reportTime);
	CHECK(expected.report == actual.report);
}

inline void verify_request_daily_report(const RequestDailyReportMessage& expected, const RequestDailyReportMessage& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.month == actual.month);
	CHECK(expected.day == actual.day);
	CHECK(expected.year == actual.year);
}

inline void verify_weekly_report(const WeeklyReportMessage& expected, const WeeklyReportMessage& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.reportTime == actual.reportTime);
	CHECK(expected.dailyReports == actual.dailyReports);
	CHECK(expected.totalTime == actual.totalTime);
	CHECK(expected.timePerTimeCode == actual.timePerTimeCode);
}

inline void verify_request_weekly_report(const RequestWeeklyReportMessage& expected, const RequestWeeklyReportMessage& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.month == actual.month);
	CHECK(expected.day == actual.day);
	CHECK(expected.year == actual.year);
}

inline void verify_time_entry_data(const TimeEntryDataPacket& expected, const TimeEntryDataPacket& actual, std::source_location location)
{
	CHECK(expected.timeCategories == actual.timeCategories);
}

inline void verify_time_entry_modify(const TimeEntryModifyPacket& expected, const TimeEntryModifyPacket& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.categories.size() == actual.categories.size());
	CHECK(expected.codes.size() == actual.codes.size());

	for (int i = 0; i < expected.categories.size() && i < actual.categories.size(); i++)
	{
		CHECK(expected.categories[i].type == actual.categories[i].type);
		CHECK(expected.categories[i].id == actual.categories[i].id);
		CHECK(expected.categories[i].name == actual.categories[i].name);
	}

	for (int i = 0; i < expected.codes.size() && i < actual.codes.size(); i++)
	{
		CHECK(expected.codes[i].type == actual.codes[i].type);
		CHECK(expected.codes[i].categoryIndex == actual.codes[i].categoryIndex);
		CHECK(expected.codes[i].codeID == actual.codes[i].codeID);
		CHECK(expected.codes[i].name == actual.codes[i].name);
		CHECK(expected.codes[i].archive == actual.codes[i].archive);
	}
}

inline void verify_update_task_times(const UpdateTaskTimesMessage& expected, const UpdateTaskTimesMessage& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.taskID == actual.taskID);
	CHECK(expected.sessionIndex == actual.sessionIndex);
	CHECK(expected.start == actual.start);
	CHECK(expected.stop == actual.stop);
}

inline void verify_task_state_change(const TaskStateChange& expected, const TaskStateChange& actual, std::source_location location)
{
	CHECK(expected.requestID == actual.requestID);
	CHECK(expected.taskID == actual.taskID);
	CHECK(expected.state == actual.state);
}

template<typename T>
void verify_message(const T& expected, const Message& actual, std::source_location location = std::source_location::current())
{
	INFO(cpptrace::generate_trace().to_string());

	UNSCOPED_INFO("packet type: " << static_cast<std::int32_t>(actual.packetType()));

	if (expected.packetType() != actual.packetType())
	{
		INFO("expected message: " << expected);
		INFO("Found message: " << actual);

		CHECK(false);

		return;
	}

	switch (expected.packetType())
	{
		using enum PacketType;

	case VERSION_REQUEST:
	case REQUEST_CONFIGURATION:
	case REQUEST_CONFIGURATION_COMPLETE:
	case BULK_TASK_UPDATE_START:
	case BULK_TASK_UPDATE_FINISH:
	case BULK_TASK_INFO_START:
	case BULK_TASK_INFO_FINISH:
	case UNSPECIFIED_TASK_ACTIVE:
		// basic type. no fields; no verification
		break;
	case VERSION:
	{
		verify_version_message(*dynamic_cast<const VersionMessage*>(&expected), static_cast<const VersionMessage&>(actual), location);
		break;
	}
	case CREATE_TASK:
	{
		verify_create_task(*dynamic_cast<const CreateTaskMessage*>(&expected), static_cast<const CreateTaskMessage&>(actual), location);
		break;
	}
	case START_TASK:
	case STOP_TASK:
	case FINISH_TASK:
	case REQUEST_TASK:
	{
		verify_task_message(*dynamic_cast<const TaskMessage*>(&expected), static_cast<const TaskMessage&>(actual), location);
		break;
	}
	case UPDATE_TASK:
	{
		verify_update_task(*dynamic_cast<const UpdateTaskMessage*>(&expected), static_cast<const UpdateTaskMessage&>(actual), location);
		break;
	}
	case TASK_STATE_CHANGE:
		{
		verify_task_state_change(*dynamic_cast<const TaskStateChange*>(&expected), static_cast<const TaskStateChange&>(actual), location);
		break;
		}
	case EDIT_TASK_SESSION:
	case ADD_TASK_SESSION:
	case REMOVE_TASK_SESSION:
	{
		verify_update_task_times(*dynamic_cast<const UpdateTaskTimesMessage*>(&expected), static_cast<const UpdateTaskTimesMessage&>(actual), location);
		break;
	}
	case SUCCESS_RESPONSE:
	{
		verify_success_response(*dynamic_cast<const SuccessResponse*>(&expected), static_cast<const SuccessResponse&>(actual), location);
		break;
	}
	case FAILURE_RESPONSE:
	{
		verify_failure_response(*dynamic_cast<const FailureResponse*>(&expected), static_cast<const FailureResponse&>(actual), location);
		break;
	}
	case TASK_INFO:
	{
		verify_task_info(*dynamic_cast<const TaskInfoMessage*>(&expected), static_cast<const TaskInfoMessage&>(actual), location);
		break;
	}
	case DAILY_REPORT:
	{
		verify_daily_report(*dynamic_cast<const DailyReportMessage*>(&expected), static_cast<const DailyReportMessage&>(actual), location);
		break;
	}
	case REQUEST_DAILY_REPORT:
	{
		verify_request_daily_report(*dynamic_cast<const RequestDailyReportMessage*>(&expected), static_cast<const RequestDailyReportMessage&>(actual), location);
		break;
	}
	case WEEKLY_REPORT:
	{
		verify_weekly_report(*dynamic_cast<const WeeklyReportMessage*>(&expected), static_cast<const WeeklyReportMessage&>(actual), location);
		break;
	}
	case REQUEST_WEEKLY_REPORT:
	{
		verify_request_weekly_report(*dynamic_cast<const RequestWeeklyReportMessage*>(&expected), static_cast<const RequestWeeklyReportMessage&>(actual), location);
		break;
	}
	case BUGZILLA_INFO:
	{
		verify_bugzilla_info(*dynamic_cast<const BugzillaInfoMessage*>(&expected), static_cast<const BugzillaInfoMessage&>(actual), location);
		break;
	}
	case TIME_ENTRY_DATA:
	{
		verify_time_entry_data(*dynamic_cast<const TimeEntryDataPacket*>(&expected), static_cast<const TimeEntryDataPacket&>(actual), location);
		break;
	}
	case TIME_ENTRY_MODIFY:
	{
		verify_time_entry_modify(*dynamic_cast<const TimeEntryModifyPacket*>(&expected), static_cast<const TimeEntryModifyPacket&>(actual), location);
		break;
	}
	default:
		FAIL("Unhandled packet type");
	}
}
