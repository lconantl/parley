#pragma once

#include <string>

class IAIClient
{
public:
	virtual ~IAIClient() = default;

	virtual std::string Complete(
		const std::string& systemPrompt,
		const std::string& userPrompt,
		bool jsonMode = false) = 0;
};