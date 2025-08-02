#pragma once

#include <cstdint>
#include <format>

#include <strong_type/strong_type.hpp>

using RequestID = strong::type<std::int32_t, struct request_id_, strong::equality, strong::incrementable, strong::ordered, strong::partially_ordered>;

template <>
struct std::formatter<RequestID> : std::formatter<std::int32_t> {
	auto format(RequestID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};