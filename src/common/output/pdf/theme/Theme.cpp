#include "Theme.hpp"
#include <stdexcept>

struct Metrics;
namespace
{
void AssertIsPositiveColumnCount(const int columnCount)
{
	if (columnCount <= 0)
	{
		throw std::invalid_argument("Количество колонок должно быть положительным");
	}
}

void AssertIsColumnIndexInRange(const int columnCount, const int columnIndex)
{
	if (columnIndex < 0 || columnIndex >= columnCount)
	{
		throw std::out_of_range("Индекс колонки выходит за пределы сетки");
	}
}

double TotalGapWidth(const Metrics& metrics, const int columnCount)
{
	return metrics.card.columnGap * static_cast<double>(columnCount - 1);
}
} // namespace

double ContentWidth(const Metrics& metrics)
{
	return metrics.page.width - metrics.page.marginLeft - metrics.page.marginRight;
}

double ColumnWidth(const Metrics& metrics, const int columnCount)
{
	AssertIsPositiveColumnCount(columnCount);

	const double available = ContentWidth(metrics) - TotalGapWidth(metrics, columnCount);

	return available / static_cast<double>(columnCount);
}

double ColumnOffset(const Metrics& metrics, const int columnCount, const int columnIndex)
{
	AssertIsPositiveColumnCount(columnCount);
	AssertIsColumnIndexInRange(columnCount, columnIndex);

	const double pitch = ColumnWidth(metrics, columnCount) + metrics.card.columnGap;

	return metrics.page.marginLeft + pitch * static_cast<double>(columnIndex);
}