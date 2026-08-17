#pragma once

#include "PptxSlide.hpp"

#include <filesystem>
#include <string>
#include <vector>

class PptxBuilder
{
public:
	explicit PptxBuilder(
		std::vector<PptxSlide> slides = {});

	void AddSlide(
		PptxSlide slide);

	void Save(
		const std::filesystem::path& filePath) const;

private:
	[[nodiscard]]
	static std::string BuildContentTypes(
		std::size_t slideCount,
		const std::vector<std::string>& imageExtensions);

	[[nodiscard]]
	static std::string BuildRootRels();

	[[nodiscard]]
	static std::string BuildCoreProps();

	[[nodiscard]]
	static std::string BuildAppProps(
		std::size_t slideCount);

	[[nodiscard]]
	static std::string BuildPresentationXml(
		std::size_t slideCount);

	[[nodiscard]]
	static std::string BuildPresentationRels(
		std::size_t slideCount);

	[[nodiscard]]
	static std::string BuildSlideMasterXml();

	[[nodiscard]]
	static std::string BuildSlideMasterRels();

	[[nodiscard]]
	static std::string BuildSlideLayoutXml();

	[[nodiscard]]
	static std::string BuildSlideLayoutRels();

	[[nodiscard]]
	static std::string BuildThemeXml();

	[[nodiscard]]
	static std::string BuildSlideRels(
		const std::vector<std::string>& imageTargets);

	[[nodiscard]]
	static std::string BuildSlideXml(
		const PptxSlide& slide,
		const std::vector<std::string>& imageRelationshipIds);

	[[nodiscard]]
	static std::string BuildPictureXml(
		int shapeId,
		const std::string& relationshipId,
		const PptxImage& image);

	[[nodiscard]]
	static std::string GetImageContentType(
		const std::string& extension);

	[[nodiscard]]
	static std::string ReadBinaryFile(
		const std::filesystem::path& path);

	[[nodiscard]]
	static long long EmuFromInches(
		double inches);

	[[nodiscard]]
	static std::string NormalizeHexColor(
		const std::string& color);

	[[nodiscard]]
	static std::string EscapeXml(
		const std::string& value);

	std::vector<PptxSlide> m_slides;
};
