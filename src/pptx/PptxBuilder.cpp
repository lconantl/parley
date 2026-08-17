#include "PptxBuilder.hpp"

#include <zip.h>

#include <sstream>
#include <stdexcept>
#include <utility>

namespace
{

constexpr const char* XML_DECLARATION = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

constexpr const char* SLIDE_MASTER_RELS = "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
										  "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>"
										  "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme\" Target=\"../theme/theme1.xml\"/>"
										  "</Relationships>";

constexpr const char* SLIDE_LAYOUT_RELS = "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
										  "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"../slideMasters/slideMaster1.xml\"/>"
										  "</Relationships>";

constexpr const char* SLIDE_RELS = "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
								   "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>"
								   "</Relationships>";

constexpr const char* ROOT_RELS = "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
								  "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"ppt/presentation.xml\"/>"
								  "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties\" Target=\"docProps/core.xml\"/>"
								  "<Relationship Id=\"rId3\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties\" Target=\"docProps/app.xml\"/>"
								  "</Relationships>";

constexpr const char* CORE_PROPS = "<cp:coreProperties"
								   " xmlns:cp=\"http://schemas.openxmlformats.org/package/2006/metadata/core-properties\""
								   " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
								   " xmlns:dcterms=\"http://purl.org/dc/terms/\""
								   " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">"
								   "<dc:title>Presentation</dc:title>"
								   "<dc:creator>Parley</dc:creator>"
								   "</cp:coreProperties>";

constexpr const char* SLIDE_MASTER_XML = "<p:sldMaster"
										 " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
										 " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\""
										 " xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
										 "<p:cSld>"
										 "<p:bg><p:bgRef idx=\"1001\"><a:schemeClr val=\"bg1\"/></p:bgRef></p:bg>"
										 "<p:spTree>"
										 "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
										 "<p:grpSpPr/>"
										 "<p:sp>"
										 "<p:nvSpPr><p:cNvPr id=\"2\" name=\"Title Placeholder\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"title\"/></p:nvPr></p:nvSpPr>"
										 "<p:spPr>"
										 "<a:xfrm><a:off x=\"457200\" y=\"274638\"/><a:ext cx=\"11277600\" cy=\"1143000\"/></a:xfrm>"
										 "</p:spPr>"
										 "</p:sp>"
										 "<p:sp>"
										 "<p:nvSpPr><p:cNvPr id=\"3\" name=\"Body Placeholder\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"body\" idx=\"1\"/></p:nvPr></p:nvSpPr>"
										 "<p:spPr>"
										 "<a:xfrm><a:off x=\"457200\" y=\"1600200\"/><a:ext cx=\"11277600\" cy=\"4800600\"/></a:xfrm>"
										 "</p:spPr>"
										 "</p:sp>"
										 "</p:spTree>"
										 "</p:cSld>"
										 "<p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/>"
										 "<p:sldLayoutIdLst><p:sldLayoutId id=\"2147483649\" r:id=\"rId1\"/></p:sldLayoutIdLst>"
										 "<p:txStyles>"
										 "<p:titleStyle><a:lvl1pPr algn=\"l\"><a:defRPr sz=\"4000\" b=\"1\"/></a:lvl1pPr></p:titleStyle>"
										 "<p:bodyStyle>"
										 "<a:lvl1pPr marL=\"342900\" indent=\"-342900\"><a:buFont typeface=\"Arial\"/><a:buChar char=\"\xE2\x80\xA2\"/><a:defRPr sz=\"2400\"/></a:lvl1pPr>"
										 "<a:lvl2pPr marL=\"742950\" indent=\"-285750\"><a:buFont typeface=\"Arial\"/><a:buChar char=\"\xE2\x80\x93\"/><a:defRPr sz=\"2000\"/></a:lvl2pPr>"
										 "</p:bodyStyle>"
										 "<p:otherStyle/>"
										 "</p:txStyles>"
										 "</p:sldMaster>";

constexpr const char* SLIDE_LAYOUT_XML = "<p:sldLayout"
										 " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
										 " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\""
										 " xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
										 " type=\"obj\" preserve=\"1\">"
										 "<p:cSld name=\"Title and Content\">"
										 "<p:spTree>"
										 "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
										 "<p:grpSpPr/>"
										 "<p:sp>"
										 "<p:nvSpPr><p:cNvPr id=\"2\" name=\"Title Placeholder\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"title\"/></p:nvPr></p:nvSpPr>"
										 "<p:spPr/>"
										 "</p:sp>"
										 "<p:sp>"
										 "<p:nvSpPr><p:cNvPr id=\"3\" name=\"Body Placeholder\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"body\" idx=\"1\"/></p:nvPr></p:nvSpPr>"
										 "<p:spPr/>"
										 "</p:sp>"
										 "</p:spTree>"
										 "</p:cSld>"
										 "<p:clrMapOvr><a:overrideClrMapping bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/></p:clrMapOvr>"
										 "</p:sldLayout>";

constexpr const char* THEME_XML = "<a:theme xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\" name=\"Parley Theme\">"
								  "<a:themeElements>"
								  "<a:clrScheme name=\"Parley\">"
								  "<a:dk1><a:sysClr val=\"windowText\" lastClr=\"000000\"/></a:dk1>"
								  "<a:lt1><a:sysClr val=\"window\" lastClr=\"FFFFFF\"/></a:lt1>"
								  "<a:dk2><a:srgbClr val=\"1F3864\"/></a:dk2>"
								  "<a:lt2><a:srgbClr val=\"E7E9EE\"/></a:lt2>"
								  "<a:accent1><a:srgbClr val=\"2E5FA3\"/></a:accent1>"
								  "<a:accent2><a:srgbClr val=\"C0504D\"/></a:accent2>"
								  "<a:accent3><a:srgbClr val=\"9BBB59\"/></a:accent3>"
								  "<a:accent4><a:srgbClr val=\"8064A2\"/></a:accent4>"
								  "<a:accent5><a:srgbClr val=\"4BACC6\"/></a:accent5>"
								  "<a:accent6><a:srgbClr val=\"F79646\"/></a:accent6>"
								  "<a:hlink><a:srgbClr val=\"0563C1\"/></a:hlink>"
								  "<a:folHlink><a:srgbClr val=\"954F72\"/></a:folHlink>"
								  "</a:clrScheme>"
								  "<a:fontScheme name=\"Parley\">"
								  "<a:majorFont><a:latin typeface=\"Calibri Light\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:majorFont>"
								  "<a:minorFont><a:latin typeface=\"Calibri\"/><a:ea typeface=\"\"/><a:cs typeface=\"\"/></a:minorFont>"
								  "</a:fontScheme>"
								  "<a:fmtScheme name=\"Parley\">"
								  "<a:fillStyleLst>"
								  "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>"
								  "<a:solidFill><a:schemeClr val=\"phClr\"><a:lumMod val=\"110000\"/></a:schemeClr></a:solidFill>"
								  "<a:solidFill><a:schemeClr val=\"phClr\"><a:lumMod val=\"105000\"/></a:schemeClr></a:solidFill>"
								  "</a:fillStyleLst>"
								  "<a:lnStyleLst>"
								  "<a:ln w=\"6350\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
								  "<a:ln w=\"12700\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
								  "<a:ln w=\"19050\" cap=\"flat\" cmpd=\"sng\" algn=\"ctr\"><a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill><a:prstDash val=\"solid\"/></a:ln>"
								  "</a:lnStyleLst>"
								  "<a:effectStyleLst>"
								  "<a:effectStyle><a:effectLst/></a:effectStyle>"
								  "<a:effectStyle><a:effectLst/></a:effectStyle>"
								  "<a:effectStyle><a:effectLst/></a:effectStyle>"
								  "</a:effectStyleLst>"
								  "<a:bgFillStyleLst>"
								  "<a:solidFill><a:schemeClr val=\"phClr\"/></a:solidFill>"
								  "<a:solidFill><a:schemeClr val=\"phClr\"><a:lumMod val=\"105000\"/></a:schemeClr></a:solidFill>"
								  "<a:solidFill><a:schemeClr val=\"phClr\"><a:lumMod val=\"110000\"/></a:schemeClr></a:solidFill>"
								  "</a:bgFillStyleLst>"
								  "</a:fmtScheme>"
								  "</a:themeElements>"
								  "</a:theme>";

std::string BuildSlidePartName(
	const std::size_t slideNumber)
{
	return "ppt/slides/slide" + std::to_string(slideNumber) + ".xml";
}

void AddEntry(
	zip_t* archive,
	const std::string& name,
	const std::string& content)
{
	zip_source_t* source = zip_source_buffer(
		archive,
		content.data(),
		content.size(),
		0);

	if (source == nullptr)
	{
		throw std::runtime_error(
			"Не удалось создать источник данных для " + name);
	}

	if (
		zip_file_add(
			archive,
			name.c_str(),
			source,
			ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8)
		< 0)
	{
		zip_source_free(source);

		throw std::runtime_error(
			"Не удалось добавить " + name + " в архив: " + zip_strerror(archive));
	}
}

} // namespace

PptxBuilder::PptxBuilder(
	std::vector<PptxSlide> slides)
	: m_slides(std::move(slides))
{
}

void PptxBuilder::AddSlide(
	PptxSlide slide)
{
	m_slides.push_back(std::move(slide));
}

void PptxBuilder::Save(
	const std::filesystem::path& filePath) const
{
	int errorCode = 0;

	zip_t* archive = zip_open(
		filePath.string().c_str(),
		ZIP_CREATE | ZIP_TRUNCATE,
		&errorCode);

	if (archive == nullptr)
	{
		throw std::runtime_error(
			"Не удалось создать pptx-файл, код ошибки libzip: " + std::to_string(errorCode));
	}

	try
	{
		std::vector<std::pair<std::string, std::string>> parts;

		parts.emplace_back("[Content_Types].xml", BuildContentTypes(m_slides.size()));
		parts.emplace_back("_rels/.rels", BuildRootRels());
		parts.emplace_back("docProps/core.xml", BuildCoreProps());
		parts.emplace_back("docProps/app.xml", BuildAppProps(m_slides.size()));
		parts.emplace_back("ppt/presentation.xml", BuildPresentationXml(m_slides.size()));
		parts.emplace_back("ppt/_rels/presentation.xml.rels", BuildPresentationRels(m_slides.size()));
		parts.emplace_back("ppt/slideMasters/slideMaster1.xml", BuildSlideMasterXml());
		parts.emplace_back("ppt/slideMasters/_rels/slideMaster1.xml.rels", BuildSlideMasterRels());
		parts.emplace_back("ppt/slideLayouts/slideLayout1.xml", BuildSlideLayoutXml());
		parts.emplace_back("ppt/slideLayouts/_rels/slideLayout1.xml.rels", BuildSlideLayoutRels());
		parts.emplace_back("ppt/theme/theme1.xml", BuildThemeXml());

		for (std::size_t index = 0; index < m_slides.size(); ++index)
		{
			const std::size_t slideNumber = index + 1;

			parts.emplace_back(
				BuildSlidePartName(slideNumber),
				BuildSlideXml(m_slides[index]));

			parts.emplace_back(
				"ppt/slides/_rels/slide" + std::to_string(slideNumber) + ".xml.rels",
				BuildSlideRels());
		}

		for (const auto& [name, content] : parts)
		{
			AddEntry(archive, name, content);
		}

		if (zip_close(archive) < 0)
		{
			throw std::runtime_error(
				std::string("Не удалось сохранить pptx-файл: ") + zip_strerror(archive));
		}
	}
	catch (...)
	{
		zip_discard(archive);
		throw;
	}
}

std::string PptxBuilder::BuildContentTypes(
	const std::size_t slideCount)
{
	std::ostringstream output;

	output
		<< XML_DECLARATION
		<< "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
		<< "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
		<< "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
		<< "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
		<< "<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml\"/>"
		<< "<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml\"/>"
		<< "<Override PartName=\"/ppt/theme/theme1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.theme+xml\"/>"
		<< "<Override PartName=\"/docProps/core.xml\" ContentType=\"application/vnd.openxmlformats-package.core-properties+xml\"/>"
		<< "<Override PartName=\"/docProps/app.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.extended-properties+xml\"/>";

	for (std::size_t slideNumber = 1; slideNumber <= slideCount; ++slideNumber)
	{
		output
			<< "<Override PartName=\"/" << BuildSlidePartName(slideNumber) << "\""
			<< " ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>";
	}

	output << "</Types>";

	return output.str();
}

std::string PptxBuilder::BuildRootRels()
{
	return XML_DECLARATION + std::string(ROOT_RELS);
}

std::string PptxBuilder::BuildCoreProps()
{
	return XML_DECLARATION + std::string(CORE_PROPS);
}

std::string PptxBuilder::BuildAppProps(
	const std::size_t slideCount)
{
	std::ostringstream output;

	output
		<< XML_DECLARATION
		<< "<Properties xmlns=\"http://schemas.openxmlformats.org/officeDocument/2006/extended-properties\""
		<< " xmlns:vt=\"http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes\">"
		<< "<Application>Parley</Application>"
		<< "<Slides>" << slideCount << "</Slides>"
		<< "</Properties>";

	return output.str();
}

std::string PptxBuilder::BuildPresentationXml(
	const std::size_t slideCount)
{
	std::ostringstream output;

	output
		<< XML_DECLARATION
		<< "<p:presentation"
		<< " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
		<< " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\""
		<< " xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
		<< "<p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId1\"/></p:sldMasterIdLst>"
		<< "<p:sldIdLst>";

	for (std::size_t index = 0; index < slideCount; ++index)
	{
		output
			<< "<p:sldId id=\"" << (256 + index) << "\""
			<< " r:id=\"rId" << (index + 2) << "\"/>";
	}

	output
		<< "</p:sldIdLst>"
		<< "<p:sldSz cx=\"12192000\" cy=\"6858000\" type=\"screen16x9\"/>"
		<< "<p:notesSz cx=\"6858000\" cy=\"9144000\"/>"
		<< "</p:presentation>";

	return output.str();
}

std::string PptxBuilder::BuildPresentationRels(
	const std::size_t slideCount)
{
	std::ostringstream output;

	output
		<< XML_DECLARATION
		<< "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
		<< "<Relationship Id=\"rId1\""
		<< " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\""
		<< " Target=\"slideMasters/slideMaster1.xml\"/>";

	for (std::size_t index = 0; index < slideCount; ++index)
	{
		output
			<< "<Relationship Id=\"rId" << (index + 2) << "\""
			<< " Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\""
			<< " Target=\"slides/slide" << (index + 1) << ".xml\"/>";
	}

	output << "</Relationships>";

	return output.str();
}

std::string PptxBuilder::BuildSlideMasterXml()
{
	return XML_DECLARATION + std::string(SLIDE_MASTER_XML);
}

std::string PptxBuilder::BuildSlideMasterRels()
{
	return XML_DECLARATION + std::string(SLIDE_MASTER_RELS);
}

std::string PptxBuilder::BuildSlideLayoutXml()
{
	return XML_DECLARATION + std::string(SLIDE_LAYOUT_XML);
}

std::string PptxBuilder::BuildSlideLayoutRels()
{
	return XML_DECLARATION + std::string(SLIDE_LAYOUT_RELS);
}

std::string PptxBuilder::BuildThemeXml()
{
	return XML_DECLARATION + std::string(THEME_XML);
}

std::string PptxBuilder::BuildSlideRels()
{
	return XML_DECLARATION + std::string(SLIDE_RELS);
}

std::string PptxBuilder::BuildSlideXml(
	const PptxSlide& slide)
{
	std::ostringstream output;

	output
		<< XML_DECLARATION
		<< "<p:sld"
		<< " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
		<< " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\""
		<< " xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\">"
		<< "<p:cSld>"
		<< "<p:spTree>"
		<< "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
		<< "<p:grpSpPr/>"
		<< "<p:sp>"
		<< "<p:nvSpPr><p:cNvPr id=\"2\" name=\"Title\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"title\"/></p:nvPr></p:nvSpPr>"
		<< "<p:spPr/>"
		<< "<p:txBody><a:bodyPr/><a:lstStyle/>"
		<< "<a:p><a:r><a:t>" << EscapeXml(slide.title) << "</a:t></a:r></a:p>"
		<< "</p:txBody>"
		<< "</p:sp>";

	if (!slide.bullets.empty())
	{
		output
			<< "<p:sp>"
			<< "<p:nvSpPr><p:cNvPr id=\"3\" name=\"Content\"/><p:cNvSpPr><a:spLocks noGrp=\"1\"/></p:cNvSpPr><p:nvPr><p:ph type=\"body\" idx=\"1\"/></p:nvPr></p:nvSpPr>"
			<< "<p:spPr/>"
			<< "<p:txBody><a:bodyPr/><a:lstStyle/>";

		for (const std::string& bullet : slide.bullets)
		{
			output << "<a:p><a:r><a:t>" << EscapeXml(bullet) << "</a:t></a:r></a:p>";
		}

		output
			<< "</p:txBody>"
			<< "</p:sp>";
	}

	output
		<< "</p:spTree>"
		<< "</p:cSld>"
		<< "<p:clrMapOvr><a:overrideClrMapping bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/></p:clrMapOvr>"
		<< "</p:sld>";

	return output.str();
}

std::string PptxBuilder::EscapeXml(
	const std::string& value)
{
	std::string result;

	result.reserve(value.size());

	for (const char character : value)
	{
		switch (character)
		{
		case '&':
			result += "&amp;";
			break;

		case '<':
			result += "&lt;";
			break;

		case '>':
			result += "&gt;";
			break;

		case '"':
			result += "&quot;";
			break;

		case '\'':
			result += "&apos;";
			break;

		default:
			result += character;
			break;
		}
	}

	return result;
}
