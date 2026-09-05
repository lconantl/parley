#include "ConsoleEncoding.hpp"
#include <stdexcept>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <clocale>
#endif

namespace
{
void AssertIsEncodingSet(const bool success)
{
	if (!success)
	{
		throw std::runtime_error("Couldn't configure the encoding of the console");
	}
}

#ifndef _WIN32
std::string SaveCurrentLocale()
{
	const char* locale = std::setlocale(LC_ALL, nullptr);
	return locale ? locale : "";
}

const char* ApplyFirstAvailableUtf8Locale()
{
	constexpr const char* candidates[] = {"C.UTF-8", "en_US.UTF-8", "C.utf8", ""};

	for (const char* candidate : candidates)
	{
		if (const char* applied = std::setlocale(LC_ALL, candidate))
		{
			return applied;
		}
	}

	return nullptr;
}
#endif
} // namespace

#ifdef _WIN32

ConsoleEncoding::ConsoleEncoding()
	: m_previousOutputCp(GetConsoleOutputCP())
	, m_previousInputCp(GetConsoleCP())
	, m_previousStdoutMode(_setmode(_fileno(stdout), _O_BINARY))
	, m_previousStderrMode(_setmode(_fileno(stderr), _O_BINARY))
{
	AssertIsEncodingSet(SetConsoleOutputCP(CP_UTF8) != 0);
	AssertIsEncodingSet(SetConsoleCP(CP_UTF8) != 0);
}

ConsoleEncoding::~ConsoleEncoding() noexcept
{
	SetConsoleOutputCP(m_previousOutputCp);
	SetConsoleCP(m_previousInputCp);
	_setmode(_fileno(stdout), m_previousStdoutMode);
	_setmode(_fileno(stderr), m_previousStderrMode);
}

#else

ConsoleEncoding::ConsoleEncoding()
	: m_previousLocale(SaveCurrentLocale())
{
	AssertIsEncodingSet(ApplyFirstAvailableUtf8Locale() != nullptr);
}

ConsoleEncoding::~ConsoleEncoding() noexcept
{
	std::setlocale(LC_ALL, m_previousLocale.c_str());
}

#endif
