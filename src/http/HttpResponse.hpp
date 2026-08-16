#pragma once

#include <string>

struct HttpResponse
{
	int statusCode = 0;
	std::string body;
};