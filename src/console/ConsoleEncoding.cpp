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

bool ContainsUtf8(const std::string& value)
{
	return value.find("UTF-8") != std::string::npos || value.find("utf8") != std::string::npos;
}

bool TrySetUtf8Locale()
{
	const char* candidates[] = {"", "C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8"};

	for (const char* candidate : candidates)
	{
		const char* applied = std::setlocale(LC_ALL, candidate);
		if (applied != nullptr && ContainsUtf8(applied))
		{
			return true;
		}
	}

	return false;
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
	AssertIsEncodingSet(TrySetUtf8Locale());
}

ConsoleEncoding::~ConsoleEncoding() noexcept
{
	std::setlocale(LC_ALL, m_previousLocale.c_str());
}

#endif
