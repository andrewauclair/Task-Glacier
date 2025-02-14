#pragma once

#include <string_view>

struct cURL
{
	virtual std::string execute_request(std::string_view url) = 0;
};
