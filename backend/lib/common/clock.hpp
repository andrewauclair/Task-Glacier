#pragma once

#include <chrono>

struct Clock
{
	virtual ~Clock() = default;

	virtual std::chrono::milliseconds now() const
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	}
};

struct TestClock : Clock
{
	std::chrono::milliseconds time = std::chrono::milliseconds(0);

	std::chrono::milliseconds now() const override
	{
		return time;
	}
};
