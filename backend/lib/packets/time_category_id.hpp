#pragma once

#include <format>

#include <strong_type/strong_type.hpp>

using TimeCategoryID = strong::type<std::int32_t, struct time_category_id_, strong::equality, strong::incrementable, strong::ordered, strong::partially_ordered>;

template <>
struct std::formatter<TimeCategoryID> : std::formatter<std::int32_t> {
	auto format(TimeCategoryID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};