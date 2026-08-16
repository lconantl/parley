#pragma once

#include "IRawParser.hpp"
#include "RawMessage.hpp"
#include <filesystem>

class TelegramExportParser final : public IRawParser
{
public:
	TelegramExportParser();
	~TelegramExportParser() override;

	std::vector<RawMessage> Parse(const std::filesystem::path& path) const override;
};