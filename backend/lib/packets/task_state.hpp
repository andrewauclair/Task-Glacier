#pragma once

#include <cstdint>

enum class TaskState : std::int32_t
{
	PENDING,
	ACTIVE,
	FINISHED
};
