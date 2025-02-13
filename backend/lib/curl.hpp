#pragma once

#include <string_view>

struct cURL
{
	virtual void execute_request(std::string_view url) = 0;
};
