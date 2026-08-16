#include "TelegramAnalyzer.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

using Json = nlohmann::json;

namespace
{
	struct Statistics
	{
		size_t totalMessages = 0;
		size_t totalChats = 0;
		std::unordered_set<std::string> uniqueUsers;
		std::unordered_map<std::string, size_t> messageTypes;
		std::unordered_set<std::string> messageFields;
		size_t textAsStringCount = 0;
		size_t textAsArrayCount = 0;
		size_t repliesCount = 0;
		size_t forwardsCount = 0;
		std::unordered_map<std::string, size_t> fileTypes;
		uintmax_t photosDirSize = 0;
		uintmax_t filesDirSize = 0;
		uintmax_t contactsDirSize = 0;
		uintmax_t jsonSize = 0;
		size_t maxTextLength = 0;
		uintmax_t largestFileSize = 0;
		std::string largestFileName;
		std::unordered_map<std::string, size_t> mediaTypes;
	};

	std::string PathToUtf8(const std::filesystem::path& path)
	{
		const auto u8str = path.u8string();
		return std::string(reinterpret_cast<const char*>(u8str.c_str()), u8str.size());
	}

	std::filesystem::path Utf8ToPath(const std::string& utf8Str)
	{
		return std::filesystem::path(reinterpret_cast<const char8_t*>(utf8Str.c_str()));
	}

	void AssertPathExists(const std::filesystem::path& path)
	{
		if (!std::filesystem::exists(path))
		{
			throw std::runtime_error("Указанный путь или файл экспорта не существует");
		}
	}

	void AssertIsFileOpen(bool isOpen)
	{
		if (!isOpen)
		{
			throw std::runtime_error("Не удалось открыть файл экспорта JSON");
		}
	}

	Json ParseExportJson(std::ifstream& file)
	{
		try
		{
			return Json::parse(file);
		}
		catch (const Json::parse_error&)
		{
			throw std::runtime_error("Ошибка парсинга: невалидный JSON файл экспорта");
		}
	}

	uintmax_t CalculateDirectorySize(const std::filesystem::path& dir)
	{
		uintmax_t size = 0;
		if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir))
		{
			for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
			{
				if (std::filesystem::is_regular_file(entry))
				{
					size += std::filesystem::file_size(entry);
				}
			}
		}
		return size;
	}

	void FindLargestFile(const std::filesystem::path& dir, uintmax_t& maxSize, std::string& maxName)
	{
		if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
		{
			return;
		}

		for (const auto& entry : std::filesystem::recursive_directory_iterator(dir))
		{
			if (std::filesystem::is_regular_file(entry))
			{
				uintmax_t currentSize = std::filesystem::file_size(entry);
				if (currentSize > maxSize)
				{
					maxSize = currentSize;
					maxName = PathToUtf8(entry.path().filename());
				}
			}
		}
	}

	size_t CalculateTextLength(const Json& textNode)
	{
		if (textNode.is_string())
		{
			return textNode.get_ref<const std::string&>().size();
		}

		if (textNode.is_array())
		{
			size_t length = 0;
			for (const auto& element : textNode)
			{
				if (element.is_string())
				{
					length += element.get_ref<const std::string&>().size();
				}
				else if (element.is_object() && element.contains("text") && element["text"].is_string())
				{
					length += element["text"].get_ref<const std::string&>().size();
				}
			}
			return length;
		}

		return 0;
	}

	void ProcessMessageText(const Json& message, Statistics& stats)
	{
		if (!message.contains("text"))
		{
			return;
		}

		const auto& textNode = message["text"];
		if (textNode.is_string())
		{
			stats.textAsStringCount++;
		}
		else if (textNode.is_array())
		{
			stats.textAsArrayCount++;
		}

		size_t textLength = CalculateTextLength(textNode);
		if (textLength > stats.maxTextLength)
		{
			stats.maxTextLength = textLength;
		}
	}

	void ProcessMessageRelations(const Json& message, Statistics& stats)
	{
		if (message.contains("reply_to_message_id"))
		{
			stats.repliesCount++;
		}
		if (message.contains("forwarded_from"))
		{
			stats.forwardsCount++;
		}
	}

void ProcessFileExtensions(const Json& message, Statistics& stats)
	{
		if (message.contains("media_type") && message["media_type"].is_string())
		{
			stats.mediaTypes[message["media_type"].get_ref<const std::string&>()]++;
		}

		auto countExtension = [&](const std::string& fieldName)
		{
			if (!message.contains(fieldName) || !message[fieldName].is_string())
				return;

			const auto& fieldValue = message[fieldName].get_ref<const std::string&>();

			if (fieldValue.find("(File not included") != std::string::npos)
				return;

			const auto filePath = Utf8ToPath(fieldValue);
			std::string ext = PathToUtf8(filePath.extension());

			if (ext.empty())
				stats.fileTypes["Без расширения"]++;
			else
				stats.fileTypes[ext]++;
		};

		countExtension("file");
		countExtension("photo");
	}

	void ProcessMessageFields(const Json& message, Statistics& stats)
	{
		for (auto iterator = message.begin(); iterator != message.end(); ++iterator)
		{
			stats.messageFields.insert(iterator.key());
		}

		if (message.contains("type") && message["type"].is_string())
		{
			stats.messageTypes[message["type"].get_ref<const std::string&>()]++;
		}

		if (message.contains("from_id") && message["from_id"].is_string())
		{
			stats.uniqueUsers.insert(message["from_id"].get_ref<const std::string&>());
		}
		else if (message.contains("actor_id") && message["actor_id"].is_string())
		{
			stats.uniqueUsers.insert(message["actor_id"].get_ref<const std::string&>());
		}

		ProcessFileExtensions(message, stats);
	}

	void ProcessMessage(const Json& message, Statistics& stats)
	{
		stats.totalMessages++;
		ProcessMessageFields(message, stats);
		ProcessMessageText(message, stats);
		ProcessMessageRelations(message, stats);
	}

void ProcessChats(const Json& root, Statistics& stats)
	{
		if (root.is_object() && root.contains("messages") && root["messages"].is_array())
		{
			stats.totalChats++;
			for (const auto& message : root["messages"])
			{
				ProcessMessage(message, stats);
			}
			return;
		}

		if (root.contains("chats") && root["chats"].is_object() &&
			root["chats"].contains("list") && root["chats"]["list"].is_array())
		{
			const auto& chatList = root["chats"]["list"];
			for (const auto& chat : chatList)
			{
				if (!chat.is_object())
					continue;

				stats.totalChats++;

				if (chat.contains("messages") && chat["messages"].is_array())
				{
					for (const auto& message : chat["messages"])
					{
						ProcessMessage(message, stats);
					}
				}
			}
			return;
		}

		if (root.is_array())
		{
			for (const auto& chat : root)
			{
				if (!chat.is_object())
					continue;

				stats.totalChats++;

				if (chat.contains("messages") && chat["messages"].is_array())
				{
					for (const auto& message : chat["messages"])
					{
						ProcessMessage(message, stats);
					}
				}
			}
			return;
		}

		throw std::runtime_error("Не удалось найти список чатов в result.json");
	}

	void ScanDirectories(const std::filesystem::path& exportDirectory, Statistics& stats)
	{
		stats.photosDirSize = CalculateDirectorySize(exportDirectory / "photos");
		stats.filesDirSize = CalculateDirectorySize(exportDirectory / "files");
		stats.contactsDirSize = CalculateDirectorySize(exportDirectory / "contacts");

		std::filesystem::path jsonPath = exportDirectory / "result.json";
		if (std::filesystem::exists(jsonPath))
		{
			stats.jsonSize = std::filesystem::file_size(jsonPath);
		}

		FindLargestFile(exportDirectory / "photos", stats.largestFileSize, stats.largestFileName);
		FindLargestFile(exportDirectory / "files", stats.largestFileSize, stats.largestFileName);
	}

	double BytesToMegabytes(uintmax_t bytes)
	{
		return static_cast<double>(bytes) / (1024.0 * 1024.0);
	}

	void PrintReport(const Statistics& stats)
	{
		std::cout << "СТАТИСТИКА ЭКСПОРТА TELEGRAM" << std::endl;
		std::cout << "Сообщений: " << stats.totalMessages << std::endl;
		std::cout << "Чатов: " << stats.totalChats << std::endl;
		std::cout << "Уникальных пользователей (ID): " << stats.uniqueUsers.size() << std::endl;

		std::cout << "\nТипы файлов (по расширениям)" << std::endl;
		for (const auto& pair : stats.fileTypes)
		{
			std::cout << (pair.first.empty() ? "Без расширения" : pair.first) << ": " << pair.second << std::endl;
		}

		std::cout << "\nТипы медиа" << std::endl;
		for (const auto& pair : stats.mediaTypes)
		{
			std::cout << pair.first << ": " << pair.second << std::endl;
		}

		std::cout << "\nПредставление текста" << std::endl;
		std::cout << "Как строка: " << stats.textAsStringCount << std::endl;
		std::cout << "Как массив (форматированный): " << stats.textAsArrayCount << std::endl;

		std::cout << "\nСвязи" << std::endl;
		std::cout << "Ответов (replies): " << stats.repliesCount << std::endl;
		std::cout << "Пересылок (forwards): " << stats.forwardsCount << std::endl;

		std::cout << "\nТипы файлов" << std::endl;
		for (const auto& [fst, snd] : stats.fileTypes)
		{
			std::cout << (fst.empty() ? "Без расширения" : fst) << ": " << snd << std::endl;
		}

		std::cout << "\nПоля в JSON" << std::endl;
		for (const auto& field : stats.messageFields)
		{
			std::cout << field << ", ";
		}
		std::cout << std::endl;

		std::cout << "\nРазмеры данных:" << std::endl;
		std::cout << "result.json: " << BytesToMegabytes(stats.jsonSize) << " MB" << std::endl;
		std::cout << "Фотографии: " << BytesToMegabytes(stats.photosDirSize) << " MB" << std::endl;
		std::cout << "Файлы: " << BytesToMegabytes(stats.filesDirSize) << " MB" << std::endl;
		std::cout << "Контакты: " << BytesToMegabytes(stats.contactsDirSize) << " MB" << std::endl;

		std::cout << "\nАномалии:" << std::endl;
		std::cout << "Самое длинное сообщение: " << stats.maxTextLength << std::endl;
		std::cout << "Самый большой файл: " << stats.largestFileName
		          << " (" << BytesToMegabytes(stats.largestFileSize) << " MB)" << std::endl;
	}
}

void TelegramAnalyzer::PrintReport(const std::filesystem::path& exportDirectory)
{
	Statistics stats;

	const std::filesystem::path jsonPath = exportDirectory / "result.json";
	AssertPathExists(jsonPath);

	std::ifstream file(jsonPath);
	AssertIsFileOpen(file.is_open());
	const Json root = ParseExportJson(file);
	ScanDirectories(exportDirectory, stats);
	ProcessChats(root, stats);

	PrintReport(stats);
}