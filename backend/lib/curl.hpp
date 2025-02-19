#pragma once

#include <string_view>

struct cURL
{
	virtual std::string execute_request(const std::string& url) const = 0;
};
