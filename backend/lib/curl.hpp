#pragma once

#include <string_view>

struct cURL
{
	virtual std::optional<std::string> execute_request(const std::string& url) = 0;
};
