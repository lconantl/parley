#pragma once

#include <string>

struct HttpResponse
{
	long statusCode = 0;
	std::string body;
};