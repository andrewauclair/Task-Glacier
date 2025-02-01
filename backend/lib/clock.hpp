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
	mutable std::chrono::milliseconds time = std::chrono::milliseconds(1737344039870); // 3:34 am, 20th January 2025

	// auto increment the clock by 15 minutes every time we call now()
	bool auto_increment_test_time = true;

	std::chrono::milliseconds now() const override
	{
		auto result = time;
		if (auto_increment_test_time)
		{
			time += std::chrono::minutes(15);
		}
		return result;
	}
};
