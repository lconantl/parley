#include "CsvContactsSource.hpp"
#include "infrastructure/import/ContactRowMapper/ContactRowMapper.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace
{
using CsvRow = std::vector<std::string>;

void AssertIsFileOpen(const std::ifstream& file, const std::filesystem::path& path)
{
	if (!file.is_open())
	{
		throw std::runtime_error("[CsvContactsSource] Не удалось открыть файл: " + path.string());
	}
}

std::string TrimCopy(const std::string& value)
{
	const auto begin = value.find_first_not_of(" \t\r\n");
	if (begin == std::string::npos)
	{
		return "";
	}

	const auto end = value.find_last_not_of(" \t\r\n");
	return value.substr(begin, end - begin + 1);
}

bool IsBlankRow(const CsvRow& row)
{
	return std::ranges::all_of(row, [](const std::string& cell) { return TrimCopy(cell).empty(); });
}

void StripUtf8Bom(std::string& content)
{
	constexpr std::string_view Bom = "\xEF\xBB\xBF";
	if (content.starts_with(Bom))
	{
		content.erase(0, Bom.size());
	}
}

std::vector<CsvRow> ParseCsv(const std::string& content)
{
	std::vector<CsvRow> rows;
	CsvRow currentRow;
	std::string field;
	bool inQuotes = false;

	std::size_t i = 0;
	const std::size_t size = content.size();

	while (i < size)
	{
		const char ch = content[i];

		if (inQuotes)
		{
			if (ch == '"')
			{
				if (i + 1 < size && content[i + 1] == '"')
				{
					field += '"';
					i += 2;
					continue;
				}
				inQuotes = false;
				++i;
				continue;
			}

			field += ch;
			++i;
			continue;
		}

		if (ch == '"')
		{
			inQuotes = true;
			++i;
			continue;
		}
		if (ch == ',')
		{
			currentRow.push_back(field);
			field.clear();
			++i;
			continue;
		}
		if (ch == '\r')
		{
			++i;
			continue;
		}
		if (ch == '\n')
		{
			currentRow.push_back(field);
			field.clear();
			rows.push_back(currentRow);
			currentRow.clear();
			++i;
			continue;
		}

		field += ch;
		++i;
	}

	if (!field.empty() || !currentRow.empty())
	{
		currentRow.push_back(field);
		rows.push_back(currentRow);
	}

	return rows;
}
} // namespace

std::vector<RawContact> CsvContactsSource::Load(const std::filesystem::path& path) const
{
	std::ifstream file(path, std::ios::binary);
	AssertIsFileOpen(file, path);

	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	StripUtf8Bom(content);

	const std::vector<CsvRow> rows = ParseCsv(content);

	std::vector<ColumnKind> columns;
	std::vector<RawContact> contacts;
	bool headerFound = false;

	for (const auto& row : rows)
	{
		if (IsBlankRow(row))
		{
			continue;
		}

		if (!headerFound)
		{
			columns.reserve(row.size());
			for (const auto& cell : row)
			{
				columns.push_back(ContactRowMapper::ClassifyHeader(cell));
			}
			headerFound = true;
			continue;
		}

		RawContact raw;
		raw.source = ContactSource::Csv;

		for (std::size_t index = 0; index < row.size() && index < columns.size(); ++index)
		{
			ContactRowMapper::ApplyColumnValue(columns[index], row[index], raw);
		}

		if (!raw.firstName.empty() || !raw.lastName.empty() || !raw.channels.empty())
		{
			contacts.push_back(raw);
		}
	}

	return contacts;
}
