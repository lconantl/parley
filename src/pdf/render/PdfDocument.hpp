#pragma once

#include <filesystem>
#include <hpdf.h>

class PdfDocument
{
public:
	PdfDocument();
	~PdfDocument();

	PdfDocument(const PdfDocument&) = delete;
	PdfDocument& operator=(const PdfDocument&) = delete;

	void Save(const std::filesystem::path& path) const;

	HPDF_Font LoadFont(const std::filesystem::path& path) const;
	HPDF_Image LoadImage(const std::filesystem::path& path) const;

	HPDF_Doc GetHandle() const;

private:
	HPDF_Doc m_pdf;
};