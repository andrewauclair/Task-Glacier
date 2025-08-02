#pragma once

#include "task.hpp"
#include "update_task.hpp"
#include "success_response.hpp"
#include "failure_response.hpp"
#include "request_daily_report.hpp"
#include "request_weekly_report.hpp"
#include "time_entry_data_packet.hpp"
#include "time_entry_modify_packet.hpp"

#include <memory>

class PacketParser
{
private:
	std::span<const std::byte> data;
	std::size_t position;

public:
	PacketParser(std::span<const std::byte> data) : data(data), position(0)
	{
	}

	template<typename T>
	std::expected<T, UnpackError> parse_next() = delete;

	template<typename T>
		requires (std::integral<T> || std::floating_point<T>) && (!std::same_as<T, bool>)
	std::expected<T, UnpackError> parse_next()
	{
		if (std::distance(data.begin() + position, data.end()) < sizeof(T))
		{
			return std::unexpected(UnpackError::NOT_ENOUGH_BYTES);
		}

		T value;
		std::memcpy(&value, data.data() + position, sizeof(T));
		value = std::byteswap(value);

		position += sizeof(T);

		return value;
	}

	template<typename T>
		requires std::is_enum_v<T>
	std::expected<T, UnpackError> parse_next()
	{
		auto result = parse_next<std::underlying_type_t<T>>();

		if (result)
		{
			return T(result.value());
		}
		return std::unexpected(result.error());
	}

	template<typename T>
		requires std::same_as<T, bool>
	std::expected<T, UnpackError> parse_next()
	{
		auto result = parse_next<std::int8_t>();

		if (result)
		{
			return result.value() != 0;
		}
		return std::unexpected(result.error());
	}

	template<typename T>
		requires strong::is_strong_type<T>::value
	std::expected<T, UnpackError> parse_next()
	{
		auto result = parse_next<strong::underlying_type_t<T>>();

		if (result)
		{
			return T(result.value());
		}
		return std::unexpected(result.error());
	}

	template<typename T>
		requires std::same_as<T, std::string>
	std::expected<T, UnpackError> parse_next()
	{
		auto length = parse_next<std::int16_t>();

		if (!length)
		{
			return std::unexpected(length.error());
		}

		if (std::distance(data.begin() + position, data.end()) < length.value())
		{
			return std::unexpected(UnpackError::NOT_ENOUGH_BYTES);
		}

		std::string name;
		name.resize(length.value());
		std::memcpy(name.data(), data.data() + position, length.value());
		position += length.value();

		return name;
	}

	template<typename T>
		requires std::same_as<T, std::chrono::milliseconds>
	std::expected<T, UnpackError> parse_next()
	{
		auto result = parse_next<std::int64_t>();

		if (result)
		{
			return std::chrono::milliseconds(result.value());
		}
		return std::unexpected(result.error());
	}

	template<typename T>
	auto parse_next_immediate()
	{
		return parse_next<T>().value();
	}
};

struct ParseResult
{
	std::unique_ptr<Message> packet;
	std::int32_t bytes_read = 0;
};

inline ParseResult parse_packet(std::span<const std::byte> bytes)
{
	ParseResult result;

	if (bytes.size() > 4)
	{
		std::int32_t raw_length;
		std::memcpy(&raw_length, bytes.data(), sizeof(raw_length));
		raw_length = std::byteswap(raw_length);

		result.bytes_read += sizeof(raw_length);

		// read out the packet type
		std::int32_t raw_type;
		std::memcpy(&raw_type, bytes.data() + result.bytes_read, sizeof(PacketType));
		const PacketType type = static_cast<PacketType>(std::byteswap(raw_type));

		result.bytes_read += sizeof(PacketType);

		switch (type)
		{
			using enum PacketType;

		case CREATE_TASK:
			result.packet = std::make_unique<CreateTaskMessage>(CreateTaskMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;

			break;
		case START_TASK:
		case STOP_TASK:
		case FINISH_TASK:
		case REQUEST_TASK:
			result.packet = std::make_unique<TaskMessage>(TaskMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case UPDATE_TASK:
			result.packet = std::make_unique<UpdateTaskMessage>(UpdateTaskMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case SUCCESS_RESPONSE:
			result.packet = std::make_unique<SuccessResponse>(SuccessResponse::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case FAILURE_RESPONSE:
			result.packet = std::make_unique<FailureResponse>(FailureResponse::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case REQUEST_CONFIGURATION:
		case REQUEST_CONFIGURATION_COMPLETE:
		case BULK_TASK_UPDATE_START:
		case BULK_TASK_UPDATE_FINISH:
		case BULK_TASK_INFO_START:
		case BULK_TASK_INFO_FINISH:
		{
			result.packet = std::make_unique<BasicMessage>(BasicMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;

			break;
		}
		case BUGZILLA_REFRESH:
		{
			result.packet = std::make_unique<RequestMessage>(RequestMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		}
		case BUGZILLA_INFO:
		{
			result.packet = std::make_unique<BugzillaInfoMessage>(BugzillaInfoMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;

			break;
		}
		case REQUEST_DAILY_REPORT:
			result.packet = std::make_unique<RequestDailyReportMessage>(RequestDailyReportMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case DAILY_REPORT:
			result.packet = std::make_unique<DailyReportMessage>(DailyReportMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case REQUEST_WEEKLY_REPORT:
			result.packet = std::make_unique<RequestWeeklyReportMessage>(RequestWeeklyReportMessage::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case TIME_ENTRY_DATA:
			result.packet = std::make_unique<TimeEntryDataPacket>(TimeEntryDataPacket::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		case TIME_ENTRY_MODIFY:
			result.packet = std::make_unique<TimeEntryModifyPacket>(TimeEntryModifyPacket::unpack(bytes.subspan(4)).value());
			result.bytes_read = raw_length;
			break;
		default:
			break;
		}
	}
	return result;
}
