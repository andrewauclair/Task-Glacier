#pragma once

#include <format>

#include <strong_type/strong_type.hpp>

using BugzillaInstanceID = strong::type<std::int32_t, struct bugzilla_instance_id_, strong::equality, strong::incrementable, strong::ordered, strong::partially_ordered>;

template <>
struct std::formatter<BugzillaInstanceID> : std::formatter<std::int32_t> {
	auto format(BugzillaInstanceID p, format_context& ctx) const {
		return std::formatter<std::int32_t>::format(p._val, ctx);
	}
};
