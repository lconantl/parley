#include "PdfDocument.hpp"
#include <stdexcept>

namespace
{
void AssertIsSuccess(const HPDF_STATUS status)
{
	if (status != HPDF_OK)
	{
		throw std::runtime_error("Внутренняя ошибка библиотеки генерации PDF");
	}
}

[[noreturn]] void HaruErrorHandler(const HPDF_STATUS errorNo, const HPDF_STATUS detailNo, void* /*userData*/)
{
	throw std::runtime_error("Критическая ошибка PDF: код " + std::to_string(errorNo) + ", детали " + std::to_string(detailNo));
}

void AssertIsPdfValid(const HPDF_Doc pdf)
{
	if (!pdf)
	{
		throw std::runtime_error("Документ PDF не инициализирован");
	}
}

void AssertIsExistingFile(const std::filesystem::path& path)
{
	if (!std::filesystem::exists(path))
	{
		throw std::invalid_argument("Файл ресурса не найден: " + path.string());
	}
}
} // namespace

PdfDocument::PdfDocument()
{
	m_pdf = HPDF_New(HaruErrorHandler, nullptr);
	AssertIsPdfValid(m_pdf);

	HPDF_UseUTFEncodings(m_pdf);
	HPDF_SetCurrentEncoder(m_pdf, "UTF-8");
	HPDF_SetCompressionMode(m_pdf, HPDF_COMP_ALL);
}

PdfDocument::~PdfDocument()
{
	if (m_pdf)
	{
		HPDF_Free(m_pdf);
	}
}

void PdfDocument::Save(const std::filesystem::path& path) const
{
	AssertIsPdfValid(m_pdf);
	const HPDF_STATUS status = HPDF_SaveToFile(m_pdf, path.string().c_str());
	AssertIsSuccess(status);
}

HPDF_Font PdfDocument::LoadFont(const std::filesystem::path& path) const
{
	AssertIsPdfValid(m_pdf);
	AssertIsExistingFile(path);

	const char* fontName = HPDF_LoadTTFontFromFile(m_pdf, path.string().c_str(), HPDF_TRUE);

	if (!fontName)
	{
		throw std::runtime_error("Не удалось загрузить шрифт");
	}

	return HPDF_GetFont(m_pdf, fontName, "UTF-8");
}

HPDF_Image PdfDocument::LoadImage(const std::filesystem::path& path) const
{
	AssertIsPdfValid(m_pdf);
	AssertIsExistingFile(path);

	const auto image = HPDF_LoadPngImageFromFile(m_pdf, path.string().c_str());

	if (!image)
	{
		throw std::runtime_error("Не удалось загрузить изображение");
	}

	return image;
}

HPDF_Doc PdfDocument::GetHandle() const
{
	AssertIsPdfValid(m_pdf);

	return m_pdf;
}