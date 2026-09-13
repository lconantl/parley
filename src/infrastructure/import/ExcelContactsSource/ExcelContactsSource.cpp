#include "ExcelContactsSource.hpp"
#include "infrastructure/import/ContactRowMapper/ContactRowMapper.hpp"

#include <xlnt/xlnt.hpp>

#include <map>
#include <stdexcept>

namespace
{
void AssertIsFileExist(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		throw std::runtime_error("[ExcelContactsSource] Файл не найден: " + path.string());
	}
}
} // namespace

std::vector<RawContact> ExcelContactsSource::Load(const std::filesystem::path& path) const
{
	AssertIsFileExist(path);

	xlnt::workbook workbook;
	workbook.load(path.string());
	const xlnt::worksheet sheet = workbook.active_sheet();

	std::map<std::uint32_t, ColumnKind> columns;
	std::vector<RawContact> contacts;

	bool isHeaderRow = true;
	for (const auto row : sheet.rows(false))
	{
		if (isHeaderRow)
		{
			for (const auto& cell : row)
			{
				columns[cell.column().index] = ContactRowMapper::ClassifyHeader(cell.to_string());
			}
			isHeaderRow = false;
			continue;
		}

		RawContact raw;
		raw.source = ContactSource::Excel;

		for (const auto& cell : row)
		{
			const auto it = columns.find(cell.column().index);
			if (it == columns.end())
			{
				continue;
			}

			ContactRowMapper::ApplyColumnValue(it->second, cell.to_string(), raw);
		}

		if (!raw.firstName.empty() || !raw.lastName.empty() || !raw.channels.empty())
		{
			contacts.push_back(raw);
		}
	}

	return contacts;
}
