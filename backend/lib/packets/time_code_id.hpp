#pragma once

#include <format>

#include <strong_type/strong_type.hpp>

using TimeCodeID = strong::type<std::int32_t, struct time_code_id_, strong::equality, strong::incrementable, strong::ordered, strong::partially_ordered>;

template <>
struct std::formatter<TimeCodeID> : std::formatter<std::int32_t> {
	auto format(TimeCodeID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};
