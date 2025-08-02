#pragma once

#include "clock.hpp"

#include <chrono>

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

	std::chrono::milliseconds midnight() const override
	{
		auto point = std::chrono::time_point<std::chrono::system_clock>(time);
		auto days = std::chrono::floor<std::chrono::days>(point).time_since_epoch();
		return std::chrono::duration_cast<std::chrono::milliseconds>(days);
	}
};
