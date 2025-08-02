#pragma once

#include <cstdint>
#include <format>

#include <strong_type/strong_type.hpp>

using TaskID = strong::type<std::int32_t, struct task_id_, strong::equality, strong::hashable, strong::ordered>;

template <>
struct std::formatter<TaskID> : std::formatter<std::int32_t> {
	auto format(TaskID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};

inline constexpr TaskID NO_PARENT = TaskID(0);